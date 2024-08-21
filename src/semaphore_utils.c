#include "semaphore_utils.h"


void semaphore_p(int sem_id) {
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1; // P operation: decrementa il semaforo
    sem_b.sem_flg = SEM_UNDO;
    //printf("Processo %d sta cercando di acquisire il semaforo...\n", getpid());
    if (semop(sem_id, &sem_b, 1) == -1) {
        exit(1);
    }
    //printf("Processo %d ha acquisito il semaforo.\n", getpid());
}

void semaphore_v(int sem_id) {
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1; // V operation: incrementa il semaforo
    sem_b.sem_flg = SEM_UNDO;
    //printf("Processo %d sta rilasciando il semaforo...\n", getpid());
    if (semop(sem_id, &sem_b, 1) == -1) {
        exit(1);
    }
    //printf("Processo %d ha rilasciato il semaforo.\n", getpid());
}
