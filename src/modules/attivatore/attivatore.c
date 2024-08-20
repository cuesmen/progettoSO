#include "attivatore.h"

static SharedMemory *shared_memory = NULL;

int get_semaphore_id(const char *shm_name) {
    key_t sem_key = ftok(shm_name, 'S');
    int sem_id = semget(sem_key, 1, 0666);
    if (sem_id == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    return sem_id;
}

int map_attivatore_shared_memory(const char *shm_name, SharedMemory **shared_memory_ptr) {
    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return -1;
    }

    void *shared_area = mmap(NULL, sizeof(Config) + 3 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_area == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return -1;
    }

    *shared_memory_ptr = (SharedMemory *)malloc(sizeof(SharedMemory));
    if (*shared_memory_ptr == NULL) {
        perror("malloc");
        munmap(shared_area, sizeof(Config) + 3 * sizeof(int));
        close(shm_fd);
        return -1;
    }

    (*shared_memory_ptr)->shared_config = (Config *)shared_area;
    (*shared_memory_ptr)->shared_free_energy = (int *)((char *)shared_area + sizeof(Config));
    (*shared_memory_ptr)->total_atoms_counter = (int *)((char *)shared_area + sizeof(Config) + sizeof(int));
    (*shared_memory_ptr)->total_atoms = (int *)((char *)shared_area + sizeof(Config) + 2 * sizeof(int));

    close(shm_fd);
    return 0;
}

int send_message_to_atoms(int msgid, long msg_type, const char *msg_text) {
    struct msg_buffer message;
    message.msg_type = msg_type;
    strncpy(message.msg_text, msg_text, sizeof(message.msg_text) - 1);
    message.msg_text[sizeof(message.msg_text) - 1] = '\0';

    while (1) {
        if (msgsnd(msgid, &message, sizeof(message.msg_text), 0) == -1) {
            if (errno == EAGAIN) {
                printf("Coda di messaggi piena, attesa prima di reinviare...\n");
                sleep(1);
                continue;
            } else {
                perror("msgsnd");
                return -1;
            }
        }
        break;
    }
    return 0;
}

void cleanup_ipc_resources(int msgid, SharedMemory *shared_memory) {
    if (msgid != -1 && msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl IPC_RMID");
    }
    if (shared_memory != NULL) {
        munmap(shared_memory->shared_config, sizeof(Config) + 3 * sizeof(int));
        free(shared_memory);
    }
}

void handle_Attivatoreerror(const char *msg, int msgid, SharedMemory *shared_memory) {
    perror(msg);
    cleanup_ipc_resources(msgid, shared_memory);
    exit(EXIT_FAILURE);
}

void init_shared_memory_and_semaphore(const char *shm_name, int *sem_id, int msgid) {
    if (map_attivatore_shared_memory(shm_name, &shared_memory) != 0) {
        handle_Attivatoreerror("Errore nella mappatura della memoria condivisa", msgid, shared_memory);
    }

    *sem_id = get_semaphore_id(shm_name);
    if (*sem_id == -1) {
        handle_Attivatoreerror("Errore nel recupero del semaforo", msgid, shared_memory);
    }
}

void init_attivatore(int sem_id, const char *shm_name) {
    semaphore_p(sem_id);
    int step_attivatore = shared_memory->shared_config->step_attivatore;
    semaphore_v(sem_id);

    if (step_attivatore <= 0) {
        fprintf(stderr, "Valore di step_attivatore non valido: %d\n", step_attivatore);
        cleanup_ipc_resources(-1, shared_memory);
        exit(EXIT_FAILURE);
    }

    printf("Attivatore avviato con shared_memory_name: %s\n", shm_name);
    printf("Valore di step_attivatore: %d\n", step_attivatore);
    sleep(2);
}

void attivatore_main_loop(int sem_id, int msgid, int step_attivatore) {
    while (1) {
        sleep(step_attivatore);

        semaphore_p(sem_id);
        int current_atoms = *(shared_memory->total_atoms_counter);
        semaphore_v(sem_id);

        if (current_atoms <= 0) {
            break;
        }

        int max_split = (current_atoms > 1) ? current_atoms / 2 : 1;
        srand(time(NULL));
        int r = rand() % max_split + 1;

        for (int i = 0; i < r; i++) {
            if (send_message_to_atoms(msgid, 1, "split") != 0) {
                handle_Attivatoreerror("Errore nell'invio del messaggio agli atomi", msgid, shared_memory);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <shared_memory_name> <msgid>\n", argv[0]);
        return 1;
    }

    const char *shm_name = argv[1];
    int msgid = atoi(argv[2]);

    int sem_id = -1;
    init_shared_memory_and_semaphore(shm_name, &sem_id, msgid);
    init_attivatore(sem_id, shm_name);

    int step_attivatore = shared_memory->shared_config->step_attivatore;
    attivatore_main_loop(sem_id, msgid, step_attivatore);

    //printf("\n\nTutti gli atomi sono terminati. Attivatore si sta spegnendo...\n\n");
    cleanup_ipc_resources(msgid, shared_memory);

    return 0;
}
