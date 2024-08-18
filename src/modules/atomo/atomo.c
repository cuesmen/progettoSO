#include "atomo.h"

static int num;
static SharedMemory *shared_memory;
int debugMode = 0;

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

int get_semaphore_id(const char *shm_name) {
    key_t sem_key = ftok(shm_name, 'S');
    int sem_id = semget(sem_key, 1, 0666);
    if (sem_id == -1) {
        perror("semget");
        exit(1);
    }
    return sem_id;
}


int map_atomo_shared_memory(const char *shm_name)
{
    int shm_fd;
    int attempts = 1;  // numero di tentativi
    while (attempts-- > 0) {
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        if (shm_fd != -1)
            break;  // se riesce ad aprire la memoria condivisa, esce dal loop
        perror("shm_open");
        sleep(1);  // aspetta un secondo prima di riprovare
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
    shared_memory->total_atoms = (int *)((char *)shared_area + sizeof(Config) + sizeof(int)  + sizeof(int));

    close(shm_fd); // Il file descriptor non è più necessario dopo la mappatura
    return 0;
}

void split_and_create_new_atomo(const char *shm_name) {
   if (num > 1){
        pid_t atomo_pid = fork();

        if (atomo_pid == 0) {  // Processo figlio
            int child_num = num / 2;
            num = child_num;  
            //printf("Ciao, sono un nuovo atomo, num = %d\n", num);
            
            // Prepara i parametri per execve
            char num_str[12];
            snprintf(num_str, sizeof(num_str), "%d", num);
            const char *const argv[] = {"./atomo", shm_name, num_str, NULL};
            
            // Esegui il nuovo programma atomo
            execve("./atomo", (char *const *)argv, NULL);
            
            // Se execve fallisce, stampa un errore e termina il processo
            perror("execve");
            exit(1);
        } else if (atomo_pid > 0) {  // Processo padre
            num = num - num / 2; 
            printDebug("Ciao, sono il padre dopo la scissione, num = %d\n", num);
            wait(NULL);
        } else {
            perror("fork");
        }
    } else {
        printDebug("Non posso scindermi ulteriormente, num = %d\n", num);
    }
}




int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <shared_memory_name> <num>\n", argv[0]);
        return 1;
    }

    const char *shm_name = argv[1];
    num = atoi(argv[2]);

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
    
    if (shared_memory == NULL || shared_memory->shared_config == NULL) {
    fprintf(stderr, "Errore nella mappatura della memoria condivisa o configurazione non valida.\n");
    return 1;
    }

    *(shared_memory->total_atoms_counter) += 1;
    *(shared_memory->total_atoms) += 1;
    *(shared_memory->shared_free_energy) += num;
    debugMode = shared_memory->shared_config->debug;
    semaphore_v(sem_id);
    
    printDebug("Ciao, sono un atomo, num = %d\n", num);
    sleep(0.5);
    split_and_create_new_atomo(shm_name);  // Se necessario
    
    semaphore_p(sem_id);
    *(shared_memory->total_atoms_counter) -= 1;
    semaphore_v(sem_id);

    munmap(shared_memory, sizeof(SharedMemory) + sizeof(int) + sizeof(int) + sizeof(int));

    return 0;
}
