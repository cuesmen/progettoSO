#include "master.h"

int create_semaphore(key_t sem_key) {
    int sem_id = semget(sem_key, 1, 0666 | IPC_CREAT);
    if (sem_id == -1) {
        perror("semget");
        return -1;
    }

    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
        struct seminfo *__buf;
    } sem_union;

    sem_union.val = 1;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1) {
        perror("semctl");
        return -1;
    }

    return sem_id;
}

int create_and_open_shared_memory(const char *shm_name, int *shm_fd) {
    *shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (*shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(*shm_fd, sizeof(Config) + sizeof(int) * 3) == -1) {
        perror("ftruncate");
        return 1;
    }
    return 0;
}

SharedMemory map_shared_memory(int shm_fd) {
    SharedMemory shared_memory;

    void *shared_area = mmap(NULL, sizeof(Config) + sizeof(int) * 3, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_area == MAP_FAILED) {
        perror("mmap");
        shared_memory.shared_config = NULL;
        return shared_memory;
    }

    shared_memory.shared_config = (Config *)shared_area;
    shared_memory.shared_free_energy = (int *)((char *)shared_area + sizeof(Config));
    shared_memory.total_atoms_counter = (int *)((char *)shared_area + sizeof(Config) + sizeof(int));
    shared_memory.total_atoms = (int *)((char *)shared_area + sizeof(Config) + sizeof(int) * 2);

    memset(shared_memory.shared_config, 0, sizeof(Config));
    *(shared_memory.shared_free_energy) = 0;
    *(shared_memory.total_atoms_counter) = 0;
    *(shared_memory.total_atoms) = 0;

    return shared_memory;
}

int create_message_queue(const char *shm_name) {
    key_t key = ftok(shm_name, 'B');
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        return -1;
    }
    return msgid;
}

void cleanup(SharedMemory shared_memory, int shm_fd, const char *shm_name, int sem_id, int msgid) {
    if (shared_memory.shared_config != NULL) {
        munmap(shared_memory.shared_config, sizeof(Config) + sizeof(int) * 3);
    }
    if (shm_fd != -1) {
        close(shm_fd);
    }
    if (shm_name != NULL) {
        shm_unlink(shm_name);
    }
    if (msgid != -1) {
        msgctl(msgid, IPC_RMID, NULL);
    }
    if (sem_id != -1) {
        semctl(sem_id, 0, IPC_RMID);
    }
}

void handle_error(const char *msg, SharedMemory shared_memory, int shm_fd, const char *shm_name, int sem_id, int msgid) {
    perror(msg);
    cleanup(shared_memory, shm_fd, shm_name, sem_id, msgid);
    exit(EXIT_FAILURE);
}

int execute_Configchild_process(const char *shm_name) {
    const char *argv[] = {"./config", shm_name, NULL};
    execve("./config", (char *const *)argv, NULL);
    perror("execve");
    exit(EXIT_FAILURE);
}

void wait_for_Configchild_process(pid_t pid) {
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        printf("Il processo config è terminato con stato %d\n", WEXITSTATUS(status));
    } else {
        printf("Il processo config è terminato in modo anomalo\n");
    }
    fflush(stdout);
}

int create_and_execute_atomo(int num, const char *shm_name, int msgid) {
    pid_t atomo_pid = fork();
    if (atomo_pid == 0) {
        char num_str[12];
        char msgid_str[12];
        snprintf(num_str, sizeof(num_str), "%d", num);
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);
        execl("./atomo", "./atomo", shm_name, num_str, msgid_str, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else if (atomo_pid > 0) {
        return 0;
    } else {
        perror("Atomo_fork");
        return 1;
    }
}

int create_and_execute_attivatore(const char *shm_name, int msgid) {
    pid_t attivatore_pid = fork();
    if (attivatore_pid == 0) {
        char msgid_str[12];
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);
        execl("./attivatore", "./attivatore", shm_name, msgid_str, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else if (attivatore_pid > 0) {
        return 0;
    } else {
        perror("Attivatore_fork");
        return 1;
    }
}

int create_and_execute_alimentazione(int step){
    pid_t alimentazione_pid = fork();
    if (alimentazione_pid == 0) {
        char step_str[10];
        snprintf(step_str, sizeof(step_str), "%d", step);

        execl("./alimentazione", "./alimentazione", step_str, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else if (alimentazione_pid > 0) {
        return 0;
    } else {
        perror("Alimentazione_fork");
        return 1;
    }
}

void init_master_shared_memory_and_semaphore(const char *shm_name, int *shm_fd, int *sem_id, SharedMemory *shared_memory) {
    printf("Creating and opening shared memory...\n");
    if (create_and_open_shared_memory(shm_name, shm_fd) != 0) {
        handle_error("create_and_open_shared_memory", *shared_memory, *shm_fd, shm_name, *sem_id, -1);
    }

    printf("Mapping shared memory...\n");
    *shared_memory = map_shared_memory(*shm_fd);
    if (shared_memory->shared_config == NULL) {
        handle_error("map_shared_memory", *shared_memory, *shm_fd, shm_name, *sem_id, -1);
    }

    printf("Creating semaphore...\n");
    key_t sem_key = ftok(shm_name, 'S');
    *sem_id = create_semaphore(sem_key);
    if (*sem_id == -1) {
        handle_error("create_semaphore", *shared_memory, *shm_fd, shm_name, *sem_id, -1);
    }
}

void init_config_process(const char *shm_name, SharedMemory shared_memory, int shm_fd, int sem_id, int msgid) {
    printf("Forking config process...\n");
    pid_t config_pid = fork();
    if (config_pid == 0) {
        execute_Configchild_process(shm_name);
    } else if (config_pid > 0) {
        wait_for_Configchild_process(config_pid);
    } else {
        handle_error("Config_fork", shared_memory, shm_fd, shm_name, sem_id, msgid);
    }
}

void init_message_queue(const char *shm_name, int *msgid, SharedMemory shared_memory, int shm_fd, int sem_id) {
    printf("Creating message queue...\n");
    *msgid = create_message_queue(shm_name);
    if (*msgid == -1) {
        handle_error("create_message_queue", shared_memory, shm_fd, shm_name, sem_id, *msgid);
    }
}

void master_main_loop(int sem_id, SharedMemory shared_memory) {
    sleep(1);
    while (1) {
        semaphore_p(sem_id);
        int current_atoms = *(shared_memory.total_atoms_counter);
        int current_energy = *(shared_memory.shared_free_energy);
        semaphore_v(sem_id);

        if (current_atoms <= 0)
            break;

        printf("Atomi in atto: %d\n", current_atoms);
        printf("Energia liberata al momento: %d\n", current_energy);
        fflush(stdout);

        sleep(1);
    }

    printf("\nTOTALE DI ATOMI CREATI: %d\n", *(int *)shared_memory.total_atoms);
    printf("ENERGIA LIBERATA: %d\n\n", *(int *)shared_memory.shared_free_energy);
}

int main() {
    const char *shm_name = "/wConfig";
    int shm_fd = -1;
    int sem_id = -1;
    int msgid = -1;
    SharedMemory shared_memory = {NULL, NULL, NULL, NULL};

    printf("\n\n=======================\n");
    printf("==========INIT=========\n");
    printf("=======================\n");

    init_master_shared_memory_and_semaphore(shm_name, &shm_fd, &sem_id, &shared_memory);

    init_config_process(shm_name, shared_memory, shm_fd, sem_id, msgid);

    init_message_queue(shm_name, &msgid, shared_memory, shm_fd, sem_id);

    printf("Activating attivatore...\n");
    if (create_and_execute_attivatore(shm_name, msgid) != 0) {
        handle_error("create_and_execute_attivatore", shared_memory, shm_fd, shm_name, sem_id, msgid);
    }

    printf("Activating alimentazione...\n");
    if (create_and_execute_alimentazione(shared_memory.shared_config->step_alimentazione) != 0) {
        handle_error("create_and_execute_alimentazione", shared_memory, shm_fd, shm_name, sem_id, msgid);
    }

    sleep(1);

    printf("\n\n=======================\n");
    printf("========STARTING=======\n");
    printf("=======================\n");

    printf("Activating atomi...\n");
    srand(time(NULL));
    for (int i = 0; i < shared_memory.shared_config->n_atomi_init; i++) {
        int num = rand() % shared_memory.shared_config->n_atom_max + 1;
        if (create_and_execute_atomo(num, shm_name, msgid) != 0) {
            handle_error("create_and_execute_atomo", shared_memory, shm_fd, shm_name, sem_id, msgid);
        }
    }

    master_main_loop(sem_id, shared_memory);

    printf("Cleaning up...\n");

    while (wait(NULL) > 0);

    cleanup(shared_memory, shm_fd, shm_name, sem_id, msgid);
    return 0;
}
