#include "alimentazione.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int step_alimentazione;  // Intervallo di tempo tra la creazione di nuovi atomi
int max_atoms;  // Numero massimo di atomi che possono essere creati
volatile sig_atomic_t stop = 0;  // Flag per fermare la creazione di atomi

// Gestore del segnale SIGCHLD per evitare processi zombie
void sigchld_handler(int)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

// Funzione per creare un nuovo processo atomo dalla funzione di alimentazione
void create_atomo_from_alimentazione(int num, const char *shm_name, int msgid)
{
    pid_t atomo_pid;
    if (stop == 0)  // Verifica se l'alimentazione non è stata fermata
    {
        atomo_pid = fork();
    }
    else {
        return;
    }

    if (atomo_pid == 0)  // Processo figlio
    {
        char num_str[12];
        char msgid_str[12];

        // Conversione dei parametri numerici in stringhe
        snprintf(num_str, sizeof(num_str), "%d", num);
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);

        // Esecuzione del processo atomo
        execl("./atomo", "./atomo", shm_name, num_str, msgid_str, NULL);
        exit(EXIT_FAILURE);  // Termina il processo in caso di errore
    }
    else if (atomo_pid < 0)  // Gestione dell'errore di fork
    {
        perror("create_atomo_from_alimentazione: fork failed");
    }
}

// Gestore del segnale per fermare l'alimentazione
void handle_signal(int)
{
    stop = 1;
}

// Funzione principale del processo di alimentazione
int main(int argc, char *argv[])
{
    if (argc < 5)  // Verifica se tutti i parametri necessari sono stati passati
    {
        printf("Usage: %s <STEP_ALIMENTAZIONE> <MAX_ATOMS> <SHM_NAME> <MSGID>\n", argv[0]);
        return 1;
    }

    // Inizializzazione dei parametri di alimentazione
    step_alimentazione = atoi(argv[1]);
    max_atoms = atoi(argv[2]);
    const char *shm_name = argv[3];
    int msgid = atoi(argv[4]);

    // Registra il gestore del segnale per terminare l'alimentazione
    signal(SIGTERM, handle_signal);

    // Configurazione del gestore SIGCHLD per evitare la creazione di processi zombie
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    // Calcolo del tempo di sleep basato su step_alimentazione
    struct timespec ts;
    ts.tv_sec = step_alimentazione / 1000000000;
    ts.tv_nsec = step_alimentazione % 1000000000;

    sleep(1);  // Attesa iniziale

    // Loop principale per la creazione degli atomi
    while (!stop)
    {
        nanosleep(&ts, NULL);  // Attende per il tempo specificato da step_alimentazione
        srand(time(NULL));

        int num = rand() % max_atoms + 1;  // Genera un numero casuale di atomi da creare
        create_atomo_from_alimentazione(num, shm_name, msgid);  // Crea gli atomi
    }

    return 0;
}
