#include "atomo.h"

static int num;
static SharedMemory *shared_memory = NULL;
int debugMode = 0;
float atomSleep = 0;
int min_n_atomico;
const char *shm_name;

void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        int sem_id = get_semaphore_id(shm_name);
        terminate_atomo(sem_id);
        printRedDebug(debugMode, "Atomo %d: Terminato a causa di un segnale\n", num);
        exit(0);
    }
}

int get_semaphore_id(const char *shm_name) {
    key_t sem_key = ftok(shm_name, 'S');
    int sem_id = semget(sem_key, 1, 0666);
    if (sem_id == -1) {
        printRed("Errore: impossibile ottenere l'ID del semaforo\n");
        exit(EXIT_FAILURE);
    }
    return sem_id;
}

int map_atomo_shared_memory(const char *shm_name, SharedMemory **shared_memory_ptr) {
    int shm_fd;
    int attempts = 3;
    while (attempts-- > 0) {
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        if (shm_fd != -1)
            break;
        printYellow("Attesa prima di riprovare a mappare la memoria condivisa...\n");
        sleep(1);
    }

    if (shm_fd == -1) {
        return -1;
    }

    void *shared_area = mmap(NULL, sizeof(Config) + 3 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_area == MAP_FAILED) {
        printRed("Errore: impossibile mappare l'area di memoria condivisa\n");
        close(shm_fd);
        return -1;
    }

    *shared_memory_ptr = (SharedMemory *)malloc(sizeof(SharedMemory));
    if (*shared_memory_ptr == NULL) {
        printRed("Errore: malloc fallito durante la mappatura della memoria condivisa\n");
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

void init_signals() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    printBlueDebug(debugMode, "Segnali di terminazione inizializzati.\n");
}

void init_shared_memory_and_semaphore(const char *shm_name, int *sem_id) {
    if (map_atomo_shared_memory(shm_name, &shared_memory) != 0) {
        handleAtomo_error("Errore nella mappatura della memoria condivisa", shm_name, -1, -1);
    }

    *sem_id = get_semaphore_id(shm_name);
    printBlueDebug(debugMode, "Memoria condivisa e semaforo inizializzati correttamente.\n");
}

void init_atom(int sem_id) {
    semaphore_p(sem_id);

    *(shared_memory->total_atoms_counter) += 1;
    *(shared_memory->total_atoms) += 1;
    debugMode = shared_memory->shared_config->debug;
    atomSleep = shared_memory->shared_config->atom_sleep;
    min_n_atomico = shared_memory->shared_config->min_n_atomico;

    semaphore_v(sem_id);

    printGreenDebug(debugMode, "Atomo %d: Inizializzato con successo.\n", num);
}

void atom_main_loop(int sem_id, int msgid) {
    struct msg_buffer message;

    while (1) {
        printBlueDebug(debugMode, "Atomo %d: In attesa di un messaggio...\n", num);
        sleep(atomSleep);
        if (msgrcv(msgid, &message, sizeof(message.msg_text), 1, 0) == -1) {
            if (errno == EIDRM || errno == ENOMSG) {
                printYellowDebug(debugMode, "Coda di messaggi rimossa o nessun messaggio disponibile. Atomo %d si sta terminando...\n", num);
                break;
            } else {
                printRed("Errore durante la ricezione del messaggio\n");
                break;
            }
        }
        if (strcmp(message.msg_text, "split") == 0) {
            printGreenDebug(debugMode, "Atomo con num %d ha ricevuto split\n", num);
            split_and_create_new_atomo(shm_name, sem_id, msgid);
        }
    }

    terminate_atomo(sem_id);
}

void split_and_create_new_atomo(const char *shm_name, int sem_id, int msgid) {
    if (num > min_n_atomico) {
        srand(time(NULL) ^ (getpid() << 16));
        int child_num = (rand() % (num - 1)) + 1;

        pid_t atomo_pid = fork();

        if (atomo_pid == 0) { 
            num = child_num;
            char num_str[12];
            snprintf(num_str, sizeof(num_str), "%d", num);
            char msgid_str[12];
            snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);
            const char *const argv[] = {"./atomo", shm_name, num_str, msgid_str, NULL};

            execve("./atomo", (char *const *)argv, NULL);
            perror("execve");
            exit(EXIT_FAILURE);
        } else if (atomo_pid > 0) { 
            num = num - child_num;
            printBlueDebug(debugMode, "Ciao, sono il padre dopo la scissione, num = %d\n", num);
            freeEnergy(num, child_num, sem_id);
        } else {
            printRed("Errore: impossibile creare il processo figlio per lo split\n");
        }
    } else {
        printYellowDebug(debugMode, "Non posso scindermi ulteriormente, num = %d\n", num);
        terminate_atomo(sem_id);
    }
}

void freeEnergy(int n1, int n2, int sem_id) {
    int toFree = 0;

    if (n1 == 1 || n2 == 1) {
        return;
    }

    if (n1 == n2) {
        toFree = (n1 * n2) - n1;
    } else {
        int mulBuf = n1 * n2;
        int max = getMax(n1, n2);
        toFree = mulBuf - max;
    }
    semaphore_p(sem_id);
    *(shared_memory->shared_free_energy) += toFree;
    semaphore_v(sem_id);
    printGreenDebug(debugMode, "Energia liberata: %d\n", toFree);
}

int getMax(int n1, int n2) {
    return (n1 > n2) ? n1 : n2;
}

void terminate_atomo(int sem_id) {
    semaphore_p(sem_id);
    if (*(shared_memory->total_atoms_counter) > 0) {
        *(shared_memory->total_atoms_counter) -= 1;
    }
    semaphore_v(sem_id);

    munmap(shared_memory->shared_config, sizeof(Config) + 3 * sizeof(int));
    free(shared_memory);
    printGreenDebug(debugMode, "Atomo %d: Terminato correttamente\n", num);
    exit(0);
}

void handleAtomo_error(const char *msg, const char *shm_name, int sem_id, int msgid) {
    printRed("%s\n", msg);
    cleanup_ipc_resources(shm_name, sem_id, msgid);
    exit(EXIT_FAILURE);
}

void cleanup_ipc_resources(const char *shm_name, int sem_id, int msgid) {
    if (shm_name != NULL) {
        shm_unlink(shm_name);
    }
    if (sem_id != -1) {
        semctl(sem_id, 0, IPC_RMID);
    }
    if (msgid != -1) {
        msgctl(msgid, IPC_RMID, NULL);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printRed("Usage: %s <shared_memory_name> <num> <msgid>\n", argv[0]);
        return 1;
    }

    shm_name = argv[1];
    num = atoi(argv[2]);
    int msgid = atoi(argv[3]);

    init_signals();
    int sem_id = -1;
    init_shared_memory_and_semaphore(shm_name, &sem_id);
    init_atom(sem_id);

    atom_main_loop(sem_id, msgid);

    return 0;
}
