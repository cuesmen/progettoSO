#include "alimentazione.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int step_alimentazione;
int max_atoms;
volatile sig_atomic_t stop = 0;

void sigchld_handler(int)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

void create_atomo_from_alimentazione(int num, const char *shm_name, int msgid)
{
    pid_t atomo_pid;
    if (stop == 0)
    {
        atomo_pid = fork();
    }
    else{
        return;
    }
    if (atomo_pid == 0)
    {
        char num_str[12];
        char msgid_str[12];

        snprintf(num_str, sizeof(num_str), "%d", num);
        snprintf(msgid_str, sizeof(msgid_str), "%d", msgid);

        execl("./atomo", "./atomo", shm_name, num_str, msgid_str, NULL);
        exit(EXIT_FAILURE);
    }
    else if (atomo_pid < 0)
    {
        perror("create_atomo_from_alimentazione: fork failed");
    }
}

void handle_signal(int)
{
    stop = 1;
}

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        printf("Usage: %s <STEP_ALIMENTAZIONE> <MAX_ATOMS> <SHM_NAME> <MSGID>\n", argv[0]);
        return 1;
    }

    step_alimentazione = atoi(argv[1]);
    max_atoms = atoi(argv[2]);
    const char *shm_name = argv[3];
    int msgid = atoi(argv[4]);

    // Registra il gestore del segnale
    signal(SIGTERM, handle_signal);

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    struct timespec ts;
    ts.tv_sec = step_alimentazione / 1000000000;
    ts.tv_nsec = step_alimentazione % 1000000000;

    sleep(1);

    while (!stop)
    {
        nanosleep(&ts, NULL);
        srand(time(NULL));

        int num = rand() % max_atoms + 1;
        // printf("Creando atomi\n");
        create_atomo_from_alimentazione(num, shm_name, msgid);
    }

    return 0;
}
