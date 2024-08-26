#include "master.h"

// Variabili globali
pid_t alimentazione_pid;
pid_t attivatore_pid;
pid_t gui_pid;
pid_t inibitore_pid;
struct rlimit limit;
int energy_demand;
int total_energy_demanded = 0;
int total_current_energy = 0;

int shm_fd = -1;
int sem_id = -1;
int msgid = -1;
const char *shm_name = "/wConfig";
SharedMemory shared_memory = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

int createdInibitore = 0;

void handle_gui_signals(int sig)
{
    if (sig == SIGRTMIN) //Attiva/Riattiva
    {
        if(createdInibitore == 1)
            kill(inibitore_pid, SIGUSR1);
        else
            create_and_execute_inibitore();
    }
    else if (sig == SIGRTMIN + 1) //Spegni
    {
        if(createdInibitore == 1)
            kill(inibitore_pid, SIGUSR2);
    }

}

void handle_sigusr1(int sig)
{
    if (sig == SIGUSR1)
    {
        kill(alimentazione_pid, SIGTERM);
        kill(attivatore_pid, SIGTERM);
        semaphore_p(sem_id);
        if (createdInibitore == 1)
            kill(inibitore_pid, SIGTERM);
        semaphore_v(sem_id);
        ExitCode exit_code = MELTDOWN;
        printRed("\nProgram exited with code: %d, meaning %s\n\n", exit_code, get_exit_code_description(exit_code));
        cleanup(); // Richiama cleanup senza argomenti
        exit(1);
    }
}

void handle_sigint(int sig)
{
    if (sig == SIGINT)
    {
        printf("\nCTRL+C received, cleaning up and exiting...\n");
        kill(alimentazione_pid, SIGTERM);
        kill(attivatore_pid, SIGTERM);
        semaphore_p(sem_id);
        if (createdInibitore == 1)
            kill(inibitore_pid, SIGTERM);
        semaphore_v(sem_id);
        cleanup();
        exit(0);
    }
}

void cleanup()
{
    if (shared_memory.shared_config != NULL)
    {
        munmap(shared_memory.shared_config, sizeof(Config) + sizeof(int) * MEMSIZE);
    }
    if (shm_fd != -1)
    {
        close(shm_fd);
    }
    if (shm_name != NULL)
    {
        shm_unlink(shm_name);
    }
    if (msgid != -1)
    {
        msgctl(msgid, IPC_RMID, NULL);
    }
    if (sem_id != -1)
    {
        semctl(sem_id, 0, IPC_RMID);
    }
    unlink("inibitore_pipe");
}

void handle_error(const char *msg)
{
    perror(msg);
    cleanup();
    exit(0);
}

int create_semaphore(key_t sem_key)
{
    int sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);
    if (sem_id == -1)
    {
        perror("semget");
        return -1;
    }

    union semun
    {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
        struct seminfo *__buf;
    } sem_union;

    sem_union.val = 1;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1)
    {
        perror("semctl");
        return -1;
    }

    return sem_id;
}

int create_and_open_shared_memory(const char *shm_name, int *shm_fd)
{
    *shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (*shm_fd == -1)
    {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(*shm_fd, sizeof(Config) + sizeof(int) * MEMSIZE) == -1)
    {
        perror("ftruncate");
        return 1;
    }
    return 0;
}

SharedMemory map_shared_memory(int shm_fd)
{
    SharedMemory shared_memory;

    void *shared_area = mmap(NULL, sizeof(Config) + sizeof(int) * MEMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_area == MAP_FAILED)
    {
        perror("mmap");
        shared_memory.shared_config = NULL;
        return shared_memory;
    }

    shared_memory.shared_config = (Config *)shared_area;
    shared_memory.shared_free_energy = (int *)((char *)shared_area + sizeof(Config));
    shared_memory.total_atoms_counter = (int *)((char *)shared_area + sizeof(Config) + sizeof(int));
    shared_memory.total_atoms = (int *)((char *)shared_area + sizeof(Config) + sizeof(int) * 2);
    shared_memory.toEnd = (int *)((char *)shared_area + sizeof(Config) + sizeof(int) * 3);
    shared_memory.total_wastes = (int *)((char *)shared_area + sizeof(Config) + sizeof(int) * 4);
    shared_memory.total_attivatore = (int *)((char *)shared_area + sizeof(Config) + sizeof(int) * 5);
    shared_memory.total_splits = (int *)((char *)shared_area + sizeof(Config) + sizeof(int) * 6);
    shared_memory.total_inibitore_energy = (int *)((char *)shared_area + sizeof(Config) + sizeof(int) * 7);
    shared_memory.inibitore_attivo = (int *)((char *)shared_area + sizeof(Config) + sizeof(int) * 8);

    memset(shared_memory.shared_config, 0, sizeof(Config));
    *(shared_memory.shared_free_energy) = 0;
    *(shared_memory.total_atoms_counter) = 0;
    *(shared_memory.total_atoms) = 0;
    *(shared_memory.toEnd) = 0;
    *(shared_memory.total_wastes) = 0;
    *(shared_memory.total_attivatore) = 0;
    *(shared_memory.total_splits) = 0;
    *(shared_memory.total_inibitore_energy) = 0;
    *(shared_memory.inibitore_attivo) = 0;

    return shared_memory;
}

int create_message_queue(const char *shm_name)
{
    key_t key = ftok(shm_name, 'B');
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        perror("msgget");
        return -1;
    }
    return msgid;
}

void wait_for_Configchild_process(pid_t pid)
{
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status))
    {
        printf("Il processo config è terminato in modo anomalo\n");
        exit(0);
    }
    fflush(stdout);
}

void init_config_process()
{
    // printf("Forking config process...\n");
    pid_t config_pid = fork();
    if (config_pid == 0)
    {
        execute_Configchild_process();
    }
    else if (config_pid > 0)
    {
        wait_for_Configchild_process(config_pid);
    }
    else
    {
        handle_error("Config_fork");
    }
}

int create_and_execute_atomo(int num)
{
    pid_t atomo_pid = fork();
    if (atomo_pid == 0)
    {
        char num_str[12];
        char msgid_str[12];
        snprintf(num_str, sizeof(num_str), "%d", num);
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);

        // Creazione dell'array di argomenti con const char *
        const char *argv[] = {"./atomo", shm_name, num_str, msgid_str, NULL};

        // Esecuzione del nuovo processo
        execve("./atomo", (char *const *)argv, NULL);
        // perror("execveAtomo");
        return 1;
    }
    else if (atomo_pid > 0)
    {
        return 0; // Il processo figlio è stato creato con successo
    }
    else
    {
        perror("Atomo_fork");
        return 1; // Fork fallito, segnala errore al processo padre
    }
}

int create_and_execute_attivatore()
{
    attivatore_pid = fork();
    if (attivatore_pid == 0)
    {
        char msgid_str[12];
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);

        // Creazione dell'array di argomenti con const char *
        const char *argv[] = {"./attivatore", shm_name, msgid_str, NULL};

        // Esecuzione del nuovo processo
        execve("./attivatore", (char *const *)argv, NULL);

        // Se execve fallisce
        perror("execveAttivatore");
        kill(getppid(), SIGUSR1);
        exit(0);
    }
    else if (attivatore_pid > 0)
    {
        return 0;
    }
    else
    {
        perror("Attivatore_fork");
        return 1;
    }
}

int create_and_execute_alimentazione(long step, int max_atoms)
{
    alimentazione_pid = fork();
    if (alimentazione_pid == 0)
    {
        char step_str[20];
        char max_str[10];
        char msgid_str[12];

        snprintf(step_str, sizeof(step_str), "%ld", step);
        snprintf(max_str, sizeof(max_str), "%d", max_atoms);
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);

        // Creazione dell'array di argomenti con const char *
        const char *argv[] = {"./alimentazione", step_str, max_str, shm_name, msgid_str, NULL};

        // Esecuzione del nuovo processo
        execve("./alimentazione", (char *const *)argv, NULL);

        // Se execve fallisce
        perror("execveAlimentazione");
        kill(getppid(), SIGUSR1);
        exit(0);
    }
    else if (alimentazione_pid > 0)
    {
        return 0;
    }
    else
    {
        perror("Alimentazione_fork");
        return 1;
    }
}

int create_and_execute_inibitore()
{
    inibitore_pid = fork();
    if (inibitore_pid == 0)
    {
        // Processo figlio: esegue l'inibitore
        const char *argv[] = {"./inibitore", shm_name, "inibitore_pipe", NULL};
        execve("./inibitore", (char *const *)argv, NULL);

        // Se execve fallisce
        perror("execveInibitore");
        kill(getppid(), SIGUSR1);
        exit(0);
    }
    else if (inibitore_pid > 0)
    {
        createdInibitore = 1;
        return 0;
    }
    else
    {
        perror("Inibitore_fork");
        return 1;
    }
}

int execute_Configchild_process()
{
    // Creazione dell'array di argomenti con const char *
    const char *argv[] = {"./config", shm_name, NULL};

    // Esecuzione del nuovo processo
    execve("./config", (char *const *)argv, NULL);

    // Se execve fallisce
    perror("execveConfig");
    kill(getppid(), SIGUSR1);
    exit(0);
}

void init_master_shared_memory_and_semaphore()
{
    // printf("Creating and opening shared memory...\n");
    if (create_and_open_shared_memory(shm_name, &shm_fd) != 0)
    {
        handle_error("create_and_open_shared_memory");
    }

    // printf("Mapping shared memory...\n");
    shared_memory = map_shared_memory(shm_fd);
    if (shared_memory.shared_config == NULL)
    {
        handle_error("map_shared_memory");
    }

    // printf("Creating semaphore...\n");
    key_t sem_key = ftok(shm_name, 'S');
    sem_id = create_semaphore(sem_key);
    if (sem_id == -1)
    {
        handle_error("create_semaphore");
    }
}

void init_message_queue()
{
    // printf("Creating message queue...\n");
    msgid = create_message_queue(shm_name);
    if (msgid == -1)
    {
        handle_error("create_message_queue");
    }
}

ExitCode master_main_loop()
{
    sleep(2);
    int counter = 0;
    int previous_energy = 0;
    int previous_wastes = 0;
    int previous_attivatore = 0;
    int previous_splits = 0;
    while (1)
    {

        if (counter == shared_memory.shared_config->sim_duration)
        {
            return TIMEOUT;
        }

        semaphore_p(sem_id);
        int current_atoms = *(shared_memory.total_atoms_counter);
        int current_energy = *(shared_memory.shared_free_energy);
        total_current_energy = current_energy;

        if(counter > 1)
            *(shared_memory.shared_free_energy) -= energy_demand;

        current_energy = *(shared_memory.shared_free_energy);
        int current_wastes = *(shared_memory.total_wastes);
        int current_attivatore = *(shared_memory.total_attivatore);
        int current_splits = *(shared_memory.total_splits);
        int total_inibitore = *(shared_memory.total_inibitore_energy);

        int deb = *(shared_memory.inibitore_attivo);

        semaphore_v(sem_id);

        if (current_atoms > (int)(limit.rlim_cur) / 2)
        {
            return EXCEDED_PROCESSES;
        }

        if (current_energy > shared_memory.shared_config->energy_explode_threshold)
        {
            return EXPLODE;
        }

        total_energy_demanded += energy_demand;
        if (total_energy_demanded > current_energy)
        {
            printRed("Quantità totale prelevata è: %d\n", total_energy_demanded);
            printRed("Quantità corrente di energia è: %d", current_energy);
            return BLACKOUT;
        }

        int created_energy = current_energy - previous_energy;
        int real_current_wastes = current_wastes - previous_wastes;
        int real_current_attivatore = current_attivatore - previous_attivatore;
        int real_current_splits = current_splits - previous_splits;

        printBlue("---------------------\n");
        printGreen("Tempo %d\n", counter);
        printf("Atomi in esecuzione: %d\n", current_atoms);
        printf("Numero di attivazioni attivatore al tempo %d: %d\n", counter, real_current_attivatore);
        printf("Numero totale di attivazioni attivatore: %d\n", current_attivatore);
        printf("Numero di scissioni al tempo %d: %d\n", counter, real_current_splits);
        printf("Numero totale di scissioni: %d\n", current_splits);
        // printf("Energia prima del prelievo: %d\n", total_current_energy);
        printf("Energia creata al tempo %d: %d\n", counter, created_energy);
        printf("Energia totale disponibile: %d\n", current_energy);
        printf("Energia consumata al tempo %d: %d\n", counter, energy_demand);
        printf("Energia totale consumata: %d\n", total_energy_demanded);
        printf("Scorie prodotte al tempo %d: %d\n", counter, real_current_wastes);
        printf("Scorie totali prodotte: %d\n", current_wastes);
        printf("Energia assorbita da inibitore: %d\n", total_inibitore);
        printf("Attivo? %d\n", deb);
        fflush(stdout);

        previous_splits = current_splits;
        previous_attivatore = current_attivatore;
        previous_wastes = current_wastes;
        previous_energy = current_energy;
        counter += 1;
        sleep(1);
    }
    return PROGRAM_ERROR;
}

const char *get_exit_code_description(ExitCode code)
{
    switch (code)
    {
    case TIMEOUT:
        return "TIMEOUT";
    case EXPLODE:
        return "EXPLODE";
    case BLACKOUT:
        return "BLACKOUT";
    case MELTDOWN:
        return "MELTDOWN";
    case EXCEDED_PROCESSES:
        return "EXCEDED_PROCESSES";
    default:
        return "PROGRAM_ERROR";
    }
}

void set_termination_flag()
{
    *(shared_memory.toEnd) = 1;
}

int main()
{

    signal(SIGRTMIN, handle_gui_signals);
    signal(SIGRTMIN + 1, handle_gui_signals);
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGINT, handle_sigint);

    if (getrlimit(RLIMIT_NPROC, &limit) == 0)
    {
        printRed("\nATTENZIONE!\nNumero massimo di processi per l'utente: %d", (int)(limit.rlim_cur) / 2);
    }
    else
    {
        perror("Errore nel recuperare il limite massimo di processi");
        return 1;
    }

    printYellow("\n\n=======================\n");
    printYellow("==========INIT=========\n");
    printYellow("=======================\n");

    init_master_shared_memory_and_semaphore();

    init_config_process();

    init_message_queue();

    printBlue("Activating attivatore...\n");
    if (create_and_execute_attivatore() != 0)
    {
        handle_error("create_and_execute_attivatore");
    }

    printBlue("Activating alimentazione...\n");
    if (create_and_execute_alimentazione(shared_memory.shared_config->step_alimentazione, shared_memory.shared_config->n_atom_max) != 0)
    {
        handle_error("create_and_execute_alimentazione");
    }

    if (shared_memory.shared_config->start_with_inibitore == 1)
    {
        printBlue("Activating inibitore...\n");
        if (create_and_execute_inibitore() != 0)
        {
            handle_error("create_and_execute_inibitore");
        }
    }

    printBlue("Getting energy demand...\n");
    energy_demand = shared_memory.shared_config->energy_demand;

    sleep(1);

    printGreen("\n\n=======================\n");
    printGreen("========STARTING=======\n");
    printGreen("=======================\n");

    printBlue("Activating atomi...\n");
    srand(time(NULL));

    for (int i = 0; i < shared_memory.shared_config->n_atomi_init; i++)
    {
        int num = rand() % shared_memory.shared_config->n_atom_max + 1;
        if (create_and_execute_atomo(num) != 0)
        {
            cleanup();
            kill(getppid(), SIGUSR1);
            break;
        }
    }

    ExitCode exit_code = master_main_loop();
    kill(alimentazione_pid, SIGTERM);
    kill(attivatore_pid, SIGTERM);

    semaphore_p(sem_id);
    if (createdInibitore == 1)
        kill(inibitore_pid, SIGTERM);
    semaphore_v(sem_id);

    set_termination_flag();

    printGreen("\nTOTALE ATOMI CREATI: %d\n", *(int *)shared_memory.total_atoms);
    printGreen("TOTALE ATTIVAZIONI EFFETTUATE: %d\n", *(int *)shared_memory.total_attivatore);
    printGreen("TOTALE SCISSIONI AVVENUTE: %d\n", *(int *)shared_memory.total_splits);
    printGreen("TOTALE ENERGIA PRELEVATA: %d\n", total_energy_demanded);
    printGreen("TOTALE ENERGIA LIBERATA: %d\n", *(int *)shared_memory.shared_free_energy);
    printGreen("TOTALE SCORIE CREATE: %d\n", *(int *)shared_memory.total_wastes);
    printGreen("TOTALE ENERGIA PRESA DA INIBITORE: %d\n\n", *(int *)shared_memory.total_inibitore_energy);

    //HARD KILLING ATOMS//
    if((shared_memory.shared_config->hard_kill_atoms) == 1){
        printBlue("Hard killing atoms...\n");
        system("pkill atomo");
    }   

    while (wait(NULL) > 0)
    ;

    printGreen("\nProgram exited with code: %d, meaning %s\n\n", exit_code, get_exit_code_description(exit_code));

    printYellow("Cleaning up all processes...\n");
    fflush(stdout);
    sleep(1);
    cleanup();
    printGreen("DONE!!\n");
    fflush(stdout);
    return 0;
}
