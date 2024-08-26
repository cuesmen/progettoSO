#ifndef ATOMO_H
#define ATOMO_H


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/mman.h>

#include <fcntl.h>

#include <errno.h>
#include <signal.h>

#include "../../structure_utils.h"
#include "../../semaphore_utils.h"
#include "../../log_utils.h"
// Funzioni IPC
int get_semaphore_id(const char *shm_name);
int map_atomo_shared_memory(const char *shm_name, SharedMemory **shared_memory);

// Funzioni di gestione del processo
void split_and_create_new_atomo(const char *shm_name, int sem_id, int msgid);
void terminate_atomo(int sem_id);
void freeEnergy(int n1, int n2, int sem_id);
int getMax(int n1, int n2);

// Gestione del segnale
void signal_handler(int signum);

// Funzioni di inizializzazione
void init_signals();
void init_shared_memory_and_semaphore(const char *shm_name, int *sem_id);
void init_atom(int sem_id);

// Funzioni di ciclo principale
void atom_main_loop(int sem_id, int msgid);

// Cleanup e gestione errori
//void handleAtomo_error(const char *msg, const char *shm_name, int sem_id, int msgid);
void handleAtomo_error(const char *msg, int sem_id);

#endif // ATOMO_H
