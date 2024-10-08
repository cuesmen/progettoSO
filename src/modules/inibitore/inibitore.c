#include "inibitore.h"

volatile sig_atomic_t active = 1; // Flag per attivazione/disattivazione inibitore
int debug = 0;
static SharedMemory *shared_memory = NULL;
int sem_id;
int msgid;

// Questa funzione gestisce i segnali che possono essere inviati al processo.
// SIGUSR1 e SIGUSR2 attivano o disattivano l'inibitore, mentre SIGTERM e SIGINT
// servono per una terminazione sicura, liberando risorse come memoria condivisa e coda di messaggi.
void handle_signal(int sig)
{
    if (sig == SIGUSR1)
    {
        active = 1; // Riattiva l'inibitore
        semaphore_p(sem_id);
        *(shared_memory->inibitore_attivo) = 1;
        semaphore_v(sem_id);
        printGreenDebug(debug, "Inibitore: Riattivato (SIGUSR1 ricevuto).\n");
    }
    else if (sig == SIGUSR2)
    {
        active = 0; // Disattiva l'inibitore
        semaphore_p(sem_id);
        *(shared_memory->inibitore_attivo) = 0;
        semaphore_v(sem_id);
        printGreenDebug(debug, "Inibitore: Disattivato (SIGUSR2 ricevuto).\n");
    }
    else if (sig == SIGTERM || sig == SIGINT)
    {
        printGreenDebug(debug, "Inibitore: Terminazione in corso (SIGTERM o SIGINT ricevuto).\n");

        // Rilascio delle risorse: qui viene liberata la memoria condivisa e rimossa la coda di messaggi.
        munmap(shared_memory->shared_config, sizeof(Config) + MEMSIZE * sizeof(int));
        free(shared_memory);

        if (msgctl(msgid, IPC_RMID, NULL) == -1)
        {
            perror("Errore nella rimozione della coda di messaggi");
        }
        else
        {
            printGreenDebug(debug, "Inibitore: Coda di messaggi rimossa correttamente.\n");
        }

        exit(0); // Termina il processo
    }
}

// Configura i gestori per i vari segnali che il processo potrebbe ricevere.
void setup_signal_handlers()
{
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    printGreenDebug(debug, "Inibitore: Gestori di segnali configurati.\n");
}

// Ottiene l'ID del semaforo associato alla memoria condivisa.
int get_semaphore_id(const char *shm_name)
{
    key_t sem_key = ftok(shm_name, 'S');
    int sem_id = semget(sem_key, 1, 0666);
    if (sem_id == -1)
    {
        printRed("Errore: impossibile ottenere l'ID del semaforo\n");
        exit(EXIT_FAILURE);
    }
    return sem_id;
}

// Mappa la memoria condivisa necessaria all'inibitore e configura i puntatori 
// alla struttura SharedMemory che conterrà i vari parametri condivisi.
int map_inibitore_shared_memory(const char *shm_name, SharedMemory **shared_memory_ptr)
{
    int shm_fd;
    int attempts = 1;
    while (attempts-- > 0)
    {
        shm_fd = shm_open(shm_name, O_RDWR, 0666);
        if (shm_fd != -1)
            break;
        sleep(1);
    }

    if (shm_fd == -1)
    {
        return -1;
    }

    void *shared_area = mmap(NULL, sizeof(Config) + MEMSIZE * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_area == MAP_FAILED)
    {
        close(shm_fd);
        return -1;
    }

    *shared_memory_ptr = (SharedMemory *)malloc(sizeof(SharedMemory));
    if (*shared_memory_ptr == NULL)
    {
        munmap(shared_area, sizeof(Config) + MEMSIZE * sizeof(int));
        close(shm_fd);
        return -1;
    }

    (*shared_memory_ptr)->shared_config = (Config *)shared_area;
    (*shared_memory_ptr)->shared_free_energy = (int *)((char *)shared_area + sizeof(Config));
    (*shared_memory_ptr)->total_atoms_counter = (int *)((char *)shared_area + sizeof(Config) + sizeof(int));
    (*shared_memory_ptr)->total_atoms = (int *)((char *)shared_area + sizeof(Config) + 2 * sizeof(int));
    (*shared_memory_ptr)->toEnd = (int *)((char *)shared_area + sizeof(Config) + 3 * sizeof(int));
    (*shared_memory_ptr)->total_wastes = (int *)((char *)shared_area + sizeof(Config) + 4 * sizeof(int));
    (*shared_memory_ptr)->total_attivatore = (int *)((char *)shared_area + sizeof(Config) + 5 * sizeof(int));
    (*shared_memory_ptr)->total_splits = (int *)((char *)shared_area + sizeof(Config) + 6 * sizeof(int));
    (*shared_memory_ptr)->total_inibitore_energy = (int *)((char *)shared_area + sizeof(Config) + 7 * sizeof(int));
    (*shared_memory_ptr)->inibitore_attivo = (int *)((char *)shared_area + sizeof(Config) + 8 * sizeof(int));

    close(shm_fd);
    return 0;
}

// Inizializza la memoria condivisa e il semaforo necessari per il funzionamento dell'inibitore.
void init_shared_memory_and_semaphore(const char *shm_name, int *sem_id)
{
    if (map_inibitore_shared_memory(shm_name, &shared_memory) != 0)
    {
        perror("Errore nella mappatura della memoria condivisa");
        exit(EXIT_FAILURE);
    }

    *sem_id = get_semaphore_id(shm_name);
    if (*sem_id == -1)
    {
        perror("Errore nel recupero del semaforo");
        exit(EXIT_FAILURE);
    }

    printGreenDebug(debug, "Inibitore: Memoria condivisa e semaforo inizializzati correttamente.\n");
}

// Funzione che restituisce un valore casuale, utilizzata per determinare
// se l'inibitore sarà attivo o meno in un determinato momento.
int getRandomReturnValue()
{
    int random_value = rand() % 10; // Genera un numero da 0 a 9
    if (random_value < 1)
    { // 0 (10% di probabilità)
        return 0;
    }
    else
    { // 1, 2, 3, 4, 5, 6, 7, 8, 9 (90% di probabilità)
        return 1;
    }
}

// Funzione principale del loop dell'inibitore. Gestisce i messaggi di tipo 1 e 3,
// che richiedono rispettivamente la riduzione dell'energia e lo stato dell'inibitore.
void inibitore_main_loop(int msgid)
{
    printGreenDebug(debug, "Inibitore: Avvio del loop principale.\n");
    struct atomo_msg_buffer message;
    while (1)
    {

        if (!active)
        {
            continue;
        }

        // Gestisce messaggi di tipo 1 (riduzione energia)
        if (msgrcv(msgid, &message, sizeof(message) - sizeof(long), 1, IPC_NOWAIT) > 0)
        {
            printGreenDebug(debug, "Inibitore: Messaggio ricevuto. Energia=%d\n", message.energia_ricevuta);
            message.energia_da_ridurre = calcola_energia_da_ridurre(message.energia_ricevuta);

            // Invia la risposta all'atomo
            message.msg_type = message.atom_pid; // Risposta al PID del processo chiamante
            if (msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1)
            {
                perror("Errore nella scrittura nella coda di messaggi");
            }
            else
            {
                printGreenDebug(debug, "Inibitore: Risposta inviata. Energia ridotta=%d\n", message.energia_da_ridurre);
            }
        }

        // Gestisce messaggi di tipo 3 (richiesta di stato)
        if (msgrcv(msgid, &message, sizeof(message) - sizeof(long), 3, IPC_NOWAIT) > 0)
        {
            int toReturn = getRandomReturnValue(); // L'inibitore ha il 10% di probabilità di ritornare un valore di inattività (0)
            printGreenDebug(debug, "INIBITORE: Ritornando stato %d\n", toReturn);
            message.msg_type = message.atom_pid; // Risposta al PID del processo chiamante
            message.energia_da_ridurre = toReturn;

            if (msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1)
            {
                perror("Errore nella scrittura nella coda di messaggi");
            }
        }
    }
}

// Calcola l'energia da ridurre in base alla situazione attuale del sistema.
int calcola_energia_da_ridurre(int energia_ricevuta)
{
    semaphore_p(sem_id);
    int energia_attuale = *(shared_memory->shared_free_energy);
    int soglia = shared_memory->shared_config->energy_explode_threshold;
    semaphore_v(sem_id);

    int energia_potenziale = energia_attuale + energia_ricevuta;
    int sogliaPercentuale = soglia - (soglia / 3);

    if (energia_potenziale >= sogliaPercentuale)
    {
        int energia_da_ridurre = energia_ricevuta;

        if (energia_da_ridurre <= 0)
        {
            energia_da_ridurre = 0;
        }

        printGreenDebug(debug, "Inibitore: Energia ricevuta=%d, Energia da ridurre=%d, Energia attuale=%d, Soglia=%d, SogliaPercentuale=%d\n",
                        energia_ricevuta, energia_da_ridurre, energia_attuale, soglia, sogliaPercentuale);
        return energia_da_ridurre;
    }
    else
    {
        int energia_da_ridurre = (int)(energia_ricevuta * 0.2);

        printGreenDebug(debug, "Inibitore: Energia ricevuta=%d, Energia da ridurre=%d, Energia attuale=%d, Soglia=%d, SogliaPercentuale=%d\n",
                        energia_ricevuta, energia_da_ridurre, energia_attuale, soglia, sogliaPercentuale);
        return energia_da_ridurre;
    }
}

// Funzione principale del processo inibitore.
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <shared_memory_name>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));
    setup_signal_handlers();

    const char *shm_name = argv[1];

    // Creazione della coda di messaggi
    msgid = msgget(MSG_KEY, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        perror("Errore nella creazione della coda di messaggi");
        return 1;
    }
    printGreenDebug(debug, "Inibitore: Coda di messaggi creata con msgid=%d.\n", msgid);

    // Inizializza memoria condivisa e semaforo
    init_shared_memory_and_semaphore(shm_name, &sem_id);

    // Imposta il flag inibitore_attivo a 1
    semaphore_p(sem_id);
    *(shared_memory->inibitore_attivo) = 1;
    semaphore_v(sem_id);

    inibitore_main_loop(msgid);

    // Pulizia e rimozione della coda di messaggi
    msgctl(msgid, IPC_RMID, NULL);
    printGreenDebug(debug, "Inibitore: Coda di messaggi rimossa e terminazione completata.\n");

    return 0;
}
