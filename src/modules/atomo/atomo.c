#include "atomo.h"

static int num;
static SharedMemory *shared_memory;
int debugMode = 0;
float atomSleep = 0;

void printDebug(const char *format, ...)
{
    if (debugMode == 1)
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

int get_semaphore_id(const char *shm_name)
{
    key_t sem_key = ftok(shm_name, 'S');
    int sem_id = semget(sem_key, 1, 0666);
    if (sem_id == -1)
    {
        perror("semget");
        exit(1);
    }
    return sem_id;
}

int map_atomo_shared_memory(const char *shm_name)
{
    int shm_fd;
    int attempts = 3;
    while (attempts-- > 0)
    {
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        if (shm_fd != -1)
            break; // se riesce ad aprire la memoria condivisa, esce dal loop
        perror("shm_open");
        sleep(1); // aspetta un secondo prima di riprovare
    }

    if (shm_fd == -1)
    {
        return 1;
    }

    // Mappa l'intera area di memoria condivisa
    void *shared_area = mmap(NULL, sizeof(Config) + 3 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_area == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    // Assegna i puntatori all'interno dell'area mappata
    shared_memory = (SharedMemory *)malloc(sizeof(SharedMemory));
    shared_memory->shared_config = (Config *)shared_area;
    shared_memory->shared_free_energy = (int *)((char *)shared_area + sizeof(Config));
    shared_memory->total_atoms_counter = (int *)((char *)shared_area + sizeof(Config) + sizeof(int));
    shared_memory->total_atoms = (int *)((char *)shared_area + sizeof(Config) + sizeof(int) + sizeof(int));

    close(shm_fd); // Il file descriptor non è più necessario dopo la mappatura
    return 0;
}

int getMax(int n1, int n2)
{
    return (n1 > n2) ? n1 : n2;
}

void freeEnergy(int n1, int n2, int sem_id)
{

    // energy(n1, n2) = n1n2 − max(n1, n2)

    int toFree = 0;

    if (n1 == 1 || n2 == 1)
    {
        return;
    }

    if (n1 == n2)
    {
        toFree = (n1 * n2) - n1;
    }
    else
    {
        int mulBuf = n1 * n2;
        int max = getMax(n1, n2);
        toFree = mulBuf - max;
    }
    semaphore_p(sem_id);
    *(shared_memory->shared_free_energy) += toFree;
    semaphore_v(sem_id);
}

void split_and_create_new_atomo(const char *shm_name, int sem_id)
{
    if (num > 1)
    {
        srand(time(NULL) ^ (getpid() << 16));
        int child_num = (rand() % (num - 1)) + 1;

        pid_t atomo_pid = fork();

        if (atomo_pid == 0)
        { // Processo figlio
            num = child_num;

            semaphore_p(sem_id);
            *(shared_memory->total_atoms_counter) += 1;
            *(shared_memory->total_atoms) += 1;
            semaphore_v(sem_id);

            char num_str[12];
            snprintf(num_str, sizeof(num_str), "%d", num);
            const char *const argv[] = {"./atomo", shm_name, num_str, NULL};

            execve("./atomo", (char *const *)argv, NULL);
            perror("execve");
            exit(1);
        }
        else if (atomo_pid > 0)
        { // Processo padre
            num = num - child_num;
            printDebug("Ciao, sono il padre dopo la scissione, num = %d\n", num);

            freeEnergy(num, child_num, sem_id);
        }
        else
        {
            perror("fork");
        }
    }
    else
    {
        printDebug("Non posso scindermi ulteriormente, num = %d\n", num);
    }
}

void cleanup_ipc_resources(const char *shm_name, int sem_id, int msgid)
{
    // Dealloca la memoria condivisa
    if (shm_unlink(shm_name) == -1)
    {
        perror("shm_unlink");
    }

    // Rimuove il semaforo
    if (semctl(sem_id, 0, IPC_RMID) == -1)
    {
        perror("semctl IPC_RMID");
    }

    // Rimuove la coda di messaggi
    if (msgctl(msgid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl IPC_RMID");
    }
}


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <shared_memory_name> <num> <msgid>\n", argv[0]);
        return 1;
    }

    const char *shm_name = argv[1];
    num = atoi(argv[2]);
    int msgid = atoi(argv[3]);

    if (map_atomo_shared_memory(shm_name) != 0)
    {
        return 1;
    }

    printDebug("Shared memory mapped successfully.\n");
    if (shared_memory == NULL)
    {
        printDebug("shared_memory is NULL.\n");
        return 1;
    }

    if (shared_memory->shared_config == NULL)
    {
        printDebug("shared_memory->shared_config is NULL.\n");
        return 1;
    }

    int sem_id = get_semaphore_id(shm_name);

    semaphore_p(sem_id);
    if (shared_memory == NULL || shared_memory->shared_config == NULL)
    {
        fprintf(stderr, "Errore nella mappatura della memoria condivisa o configurazione non valida.\n");
        return 1;
    }

    *(shared_memory->total_atoms_counter) += 1;
    *(shared_memory->total_atoms) += 1;
    debugMode = shared_memory->shared_config->debug;
    atomSleep = shared_memory->shared_config->atom_sleep;
    semaphore_v(sem_id);

    printDebug("Ciao, sono un atomo, num = %d\n", num);
    sleep(atomSleep);

    /**********/
    // split_and_create_new_atomo(shm_name, sem_id);
    /**********/

    struct msg_buffer message;

    while (1)
    {
        printDebug("Atomo %d: In attesa di un messaggio...\n", num);
        if (msgrcv(msgid, &message, sizeof(message.msg_text), 1, 0) == -1)
        { // '1' tipo generico
            perror("msgrcv");
            return 1;
        }
        if (strcmp(message.msg_text, "split") == 0)
        {
            printDebug("\nAtomo con num %d, ha ricevuto split\n\n", num);
            split_and_create_new_atomo(shm_name, sem_id);
        }
    }

    semaphore_p(sem_id);
    *(shared_memory->total_atoms_counter) -= 1;
    semaphore_v(sem_id);

    munmap(shared_memory, sizeof(SharedMemory) + sizeof(int) + sizeof(int) + sizeof(int));
    cleanup_ipc_resources(shm_name, sem_id, msgid);

    return 0;
}
