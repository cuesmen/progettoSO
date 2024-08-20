#include "master.h"

pid_t alimentazione_pid;
pid_t attivatore_pid;
struct rlimit limit;
int energy_demand;
int total_energy_demanded = 0;
int total_current_energy = 0;

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
    if (ftruncate(*shm_fd, sizeof(Config) + sizeof(int) * 4) == -1)
    {
        perror("ftruncate");
        return 1;
    }
    return 0;
}

SharedMemory map_shared_memory(int shm_fd)
{
    SharedMemory shared_memory;

    void *shared_area = mmap(NULL, sizeof(Config) + sizeof(int) * 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
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

    memset(shared_memory.shared_config, 0, sizeof(Config));
    *(shared_memory.shared_free_energy) = 0;
    *(shared_memory.total_atoms_counter) = 0;
    *(shared_memory.total_atoms) = 0;
    *(shared_memory.toEnd) = 0;

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

void cleanup(SharedMemory shared_memory, int shm_fd, const char *shm_name, int sem_id, int msgid)
{
    if (shared_memory.shared_config != NULL)
    {
        munmap(shared_memory.shared_config, sizeof(Config) + sizeof(int) * 4);
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
}

void handle_error(const char *msg, SharedMemory shared_memory, int shm_fd, const char *shm_name, int sem_id, int msgid)
{
    perror(msg);
    cleanup(shared_memory, shm_fd, shm_name, sem_id, msgid);
    exit(MELTDOWN);
}

int execute_Configchild_process(const char *shm_name)
{
    const char *argv[] = {"./config", shm_name, NULL};
    execve("./config", (char *const *)argv, NULL);
    perror("execveConfig");
    exit(MELTDOWN);
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

int create_and_execute_atomo(int num, const char *shm_name, int msgid)
{
    pid_t atomo_pid = fork();
    if (atomo_pid == 0)
    {
        char num_str[12];
        char msgid_str[12];
        snprintf(num_str, sizeof(num_str), "%d", num);
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);
        execl("./atomo", "./atomo", shm_name, num_str, msgid_str, NULL);
        perror("execl");
        exit(MELTDOWN);
    }
    else if (atomo_pid > 0)
    {
        return 0;
    }
    else
    {
        perror("Atomo_fork");
        return 1;
    }
}

int create_and_execute_attivatore(const char *shm_name, int msgid)
{
    attivatore_pid = fork();
    if (attivatore_pid == 0)
    {
        char msgid_str[12];
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);
        execl("./attivatore", "./attivatore", shm_name, msgid_str, NULL);
        perror("execl");
        exit(MELTDOWN);
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

int create_and_execute_alimentazione(long step, int max_atoms, const char *shm_name, int msgid)
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

        execl("./alimentazione", "./alimentazione", step_str, max_str, shm_name, msgid_str, NULL);
        perror("execl");
        exit(MELTDOWN);
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

void init_master_shared_memory_and_semaphore(const char *shm_name, int *shm_fd, int *sem_id, SharedMemory *shared_memory)
{
    //printf("Creating and opening shared memory...\n");
    if (create_and_open_shared_memory(shm_name, shm_fd) != 0)
    {
        handle_error("create_and_open_shared_memory", *shared_memory, *shm_fd, shm_name, *sem_id, -1);
    }

    //printf("Mapping shared memory...\n");
    *shared_memory = map_shared_memory(*shm_fd);
    if (shared_memory->shared_config == NULL)
    {
        handle_error("map_shared_memory", *shared_memory, *shm_fd, shm_name, *sem_id, -1);
    }

    //printf("Creating semaphore...\n");
    key_t sem_key = ftok(shm_name, 'S');
    *sem_id = create_semaphore(sem_key);
    if (*sem_id == -1)
    {
        handle_error("create_semaphore", *shared_memory, *shm_fd, shm_name, *sem_id, -1);
    }
}

void init_config_process(const char *shm_name, SharedMemory shared_memory, int shm_fd, int sem_id, int msgid)
{
    //printf("Forking config process...\n");
    pid_t config_pid = fork();
    if (config_pid == 0)
    {
        execute_Configchild_process(shm_name);
    }
    else if (config_pid > 0)
    {
        wait_for_Configchild_process(config_pid);
    }
    else
    {
        handle_error("Config_fork", shared_memory, shm_fd, shm_name, sem_id, msgid);
    }
}

void init_message_queue(const char *shm_name, int *msgid, SharedMemory shared_memory, int shm_fd, int sem_id)
{
    //printf("Creating message queue...\n");
    *msgid = create_message_queue(shm_name);
    if (*msgid == -1)
    {
        handle_error("create_message_queue", shared_memory, shm_fd, shm_name, sem_id, *msgid);
    }
}

ExitCode master_main_loop(int sem_id, SharedMemory shared_memory)
{
    sleep(1);
    int counter = 0;
    while (1)
    {

        if(counter == shared_memory.shared_config->sim_duration){
            return TIMEOUT;
        }

        semaphore_p(sem_id);
        int current_atoms = *(shared_memory.total_atoms_counter);
        int current_energy = *(shared_memory.shared_free_energy);
        total_current_energy = current_energy;
        *(shared_memory.shared_free_energy) -= energy_demand; 
        current_energy = *(shared_memory.shared_free_energy);
        semaphore_v(sem_id);

        if (current_atoms > (int)(limit.rlim_cur) / 2)
        {
            return EXCEDED_PROCESSES;
        }

        if(current_energy > shared_memory.shared_config->energy_explode_threshold){
            return EXPLODE;
        }

        total_energy_demanded += energy_demand;
        if(total_energy_demanded > current_energy){
            printRed("Quantità totale prelevata è: %d\n",total_energy_demanded);
            printRed("Quantità corrente di energia è: %d",current_energy);
            return BLACKOUT;
        }

        printBlue("---------------------\n");
        printf("Tempo %d\n",counter);
        printf("Atomi in atto: %d\n", current_atoms);
        printf("Energia prima del prelievo: %d\n", total_current_energy);
        printf("Energia disponibile al momento: %d\n", current_energy);
        printf("Energia prelevata al momento: %d\n", total_energy_demanded);
        fflush(stdout);

        counter += 1;
        sleep(1);
    }
    return PROGRAM_ERROR;
}

const char* get_exit_code_description(ExitCode code) {
    switch (code) {
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

void set_termination_flag(SharedMemory *shared){
   *(shared->toEnd) = 1;
}

int main()
{

    if (getrlimit(RLIMIT_NPROC, &limit) == 0) {
        printRed("\nATTENZIONE!\nNumero massimo di processi per l'utente: %d", (int)(limit.rlim_cur) / 2);
    } else {
        perror("Errore nel recuperare il limite massimo di processi");
        return 1;
    }

    const char *shm_name = "/wConfig";
    int shm_fd = -1;
    int sem_id = -1;
    int msgid = -1;
    SharedMemory shared_memory = {NULL, NULL, NULL, NULL, NULL};

    printYellow("\n\n=======================\n");
    printYellow("==========INIT=========\n");
    printYellow("=======================\n");

    init_master_shared_memory_and_semaphore(shm_name, &shm_fd, &sem_id, &shared_memory);

    init_config_process(shm_name, shared_memory, shm_fd, sem_id, msgid);

    init_message_queue(shm_name, &msgid, shared_memory, shm_fd, sem_id);

    printBlue("Activating attivatore...\n");
    if (create_and_execute_attivatore(shm_name, msgid) != 0)
    {
        handle_error("create_and_execute_attivatore", shared_memory, shm_fd, shm_name, sem_id, msgid);
    }

    printBlue("Activating alimentazione...\n");
    if (create_and_execute_alimentazione(shared_memory.shared_config->step_alimentazione, shared_memory.shared_config->n_atom_max, shm_name, msgid) != 0)
    {
        handle_error("create_and_execute_alimentazione", shared_memory, shm_fd, shm_name, sem_id, msgid);
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
        if (create_and_execute_atomo(num, shm_name, msgid) != 0)
        {
            handle_error("create_and_execute_atomo", shared_memory, shm_fd, shm_name, sem_id, msgid);
        }
    }

    ExitCode exit_code = master_main_loop(sem_id, shared_memory); 
    kill(alimentazione_pid, SIGTERM);
    kill(attivatore_pid, SIGTERM);
    
    set_termination_flag(&shared_memory);

    printGreen("\nTOTALE DI ATOMI CREATI: %d\n", *(int *)shared_memory.total_atoms);
    printGreen("TOTALE ENERGIA PRELEVATA: %d\n", total_energy_demanded);
    printGreen("TOTALE ENERGIA LIBERATA: %d\n\n", *(int *)shared_memory.shared_free_energy);

    while (wait(NULL) > 0);
    printGreen("\nProgram exited with code: %d, meaning %s\n\n",exit_code,get_exit_code_description(exit_code));

    printYellow("Cleaning up all processes...\n");
    fflush(stdout);
    sleep(1);
    cleanup(shared_memory, shm_fd, shm_name, sem_id, msgid);
    printGreen("DONE!!\n");
    fflush(stdout);
    return 0;
}
