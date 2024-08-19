#include "master.h"

int create_semaphore(key_t sem_key)
{
    int sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);
    if (sem_id == -1)
    {
        perror("semget");
        return -1;
    }

    // Inizializza il semaforo a 1 (mutual exclusion)
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
    if (ftruncate(*shm_fd, sizeof(Config) + sizeof(int) * 2) == -1)
    {
        perror("ftruncate");
        return 1;
    }
    return 0;
}

SharedMemory map_shared_memory(int shm_fd)
{
    SharedMemory shared_memory;

    // Mappa l'intera area di memoria condivisa
    void *shared_area = mmap(NULL, sizeof(Config) + sizeof(int) * 3, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_area == MAP_FAILED)
    {
        perror("mmap");
        shared_memory.shared_config = NULL;
        shared_memory.shared_free_energy = NULL;
        shared_memory.total_atoms_counter = NULL;
        shared_memory.total_atoms = NULL;
        return shared_memory;
    }

    // Assegna i puntatori all'interno dell'area mappata
    shared_memory.shared_config = (Config *)shared_area;
    shared_memory.shared_free_energy = (int *)((char *)shared_area + sizeof(Config));
    shared_memory.total_atoms_counter = (int *)((char *)shared_area + sizeof(Config) + sizeof(int));
    shared_memory.total_atoms = (int *)((char *)shared_area + sizeof(Config) + sizeof(int) + sizeof(int));

    // Inizializza Config, free_energy e total_atoms_counter
    memset(shared_memory.shared_config, 0, sizeof(Config));
    *(shared_memory.shared_free_energy) = 0;
    *(shared_memory.total_atoms_counter) = 0;
    *(shared_memory.total_atoms) = 0;

    return shared_memory;
}

void cleanup(SharedMemory shared_memory, int shm_fd, const char *shm_name, int sem_id)
{
    if (shared_memory.shared_config != NULL)
    {
        munmap(shared_memory.shared_config, sizeof(Config) + sizeof(int) * 3);
    }
    close(shm_fd);
    shm_unlink(shm_name);

    if (semctl(sem_id, 0, IPC_RMID) == -1)
    {
        perror("semctl IPC_RMID");
    }
}

int execute_Configchild_process(const char *shm_name)
{
    const char *argv[] = {"./config", shm_name, NULL};
    execve("./config", (char *const *)argv, NULL);
    perror("execve");
    return 1;
}

void wait_for_Configchild_process(pid_t pid)
{
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
    {
        printf("Il processo config è terminato con stato %d\n", WEXITSTATUS(status));
        fflush(stdout);
    }
    else
    {
        printf("Il processo config è terminato in modo anomalo\n");
        fflush(stdout);
    }
}

int create_and_execute_atomo(int num, const char *shm_name, int msgid) {
    pid_t atomo_pid;

    atomo_pid = fork();
    if (atomo_pid == 0)
    {
        // Processo figlio
        char num_str[12];
        char msgid_str[12];
        snprintf(num_str, sizeof(num_str), "%d", num);
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);
        execl("./atomo", "./atomo", shm_name, num_str, msgid_str, NULL);
        perror("execl");
        return 1;
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


int create_and_execute_attivatore(const char *shm_name, int msgid){
    pid_t attivatore_pid;

    attivatore_pid = fork();
    if (attivatore_pid == 0)
    {
        char msgid_str[12];
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);
        execl("./attivatore", "./attivatore", shm_name, msgid_str, NULL);
        perror("execl");
        return 1;
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


int create_message_queue(const char *shm_name)
{
    key_t key = ftok(shm_name, 'B'); // 'B' è un identificatore arbitrario
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        perror("msgget");
        return -1;
    }
    return msgid;
}

SharedMemory shared_memory;

int main()
{
    const char *shm_name = "/wConfig";
    int shm_fd;

    printf("\n\n=======================\n");
    printf("==========INIT=========\n");
    printf("=======================\n");

    printf("Creating and opening shared memory...\n");
    fflush(stdout);
    if (create_and_open_shared_memory(shm_name, &shm_fd) != 0)
    {
        return 1;
    }

    printf("Mapping shared memory...\n");
    fflush(stdout);
    shared_memory = map_shared_memory(shm_fd);
    if (shared_memory.shared_config == NULL || shared_memory.shared_free_energy == NULL)
    {
        return 1;
    }

    printf("Creating semaphore...\n");
    fflush(stdout);
    int sem_id;
    key_t sem_key = ftok(shm_name, 'S');
    sem_id = create_semaphore(sem_key);

    if (sem_id == -1)
    {
        return 1;
    }

    printf("Forking config process...\n");
    fflush(stdout);
    pid_t config_pid = fork();
    if (config_pid == 0)
    {
        return execute_Configchild_process(shm_name);
    }
    else if (config_pid > 0)
    {
        wait_for_Configchild_process(config_pid);
    }
    else
    {
        perror("Config_fork");
        return 1;
    }

    printf("Creating message queue...\n");
    fflush(stdout);
    int msgid = create_message_queue(shm_name);
    if (msgid == -1)
    {
        return 1;
    }

    printf("Activating attivatore...\n");
    if (create_and_execute_attivatore(shm_name, msgid) != 0)
    {
        printf("ERROR ACTIVATING ATTIVATORE!!");
        return 1;
    }
    sleep(1);

    printf("\n\n=======================\n");
    printf("========STARTING=======\n");
    printf("=======================\n");

    printf("Activating atomi...\n");

    srand(time(NULL));
    for (int i = 0; i < shared_memory.shared_config->n_atomi_init; i++)
    {
        int num = rand() % shared_memory.shared_config->n_atom_max + 1;
        if (create_and_execute_atomo(num, shm_name, msgid) != 0)
        {
            return 1;
        }
    }

    /*MAINLOOP*/

    sleep(1);
    while (1)
    {
        semaphore_p(sem_id);
        int current_atoms = *(shared_memory.total_atoms);
        semaphore_v(sem_id);
        if (current_atoms <= 0)
            break;
        else
        {
            printf("Atomi in atto: %d\n", current_atoms);
            fflush(stdout);
            sleep(1);
        }
    }

    /*END*/

    printf("\nTOTALE DI ATOMI CREATI: %d\n", *(int *)shared_memory.total_atoms);
    printf("ENERGIA LIBERATA: %d\n\n", *(int *)shared_memory.shared_free_energy);
    printf("Cleaning up...\n");
    cleanup(shared_memory, shm_fd, shm_name, sem_id);

    return 0;
}
