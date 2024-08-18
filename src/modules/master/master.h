#ifndef MASTER_H
#define MASTER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "../config/config.h"
#include "../semaphore_utils.h"

typedef struct {
    Config *shared_config;
    int *shared_free_energy;
    int *total_atoms_counter;
    int *total_atoms;
} SharedMemory;

// Funzione per creare e aprire la memoria condivisa
int create_and_open_shared_memory(const char *shm_name, int *shm_fd);

// Funzione per mappare la memoria condivisa
SharedMemory map_shared_memory(int shm_fd);

// Funzione per eseguire il processo figlio di configurazione
int execute_Configchild_process(const char *shm_name);

// Funzione per attendere la fine del processo figlio di configurazione
void wait_for_Configchild_process(pid_t config_pid);

// Funzione per la pulizia della memoria condivisa
void cleanup(SharedMemory shared_memory, int shm_fd, const char *shm_name, int sem_id);

// Funzione per creare e eseguire un processo atomo
int create_and_execute_atomo(int num, const char *shm_name);

int create_semaphore(key_t sem_key);

#endif // MASTER_H
