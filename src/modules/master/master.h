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
#include <sys/msg.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "../../structure_utils.h"
#include "../../semaphore_utils.h"

// Funzioni IPC
int create_semaphore(key_t sem_key);
int create_and_open_shared_memory(const char *shm_name, int *shm_fd);
SharedMemory map_shared_memory(int shm_fd);
int create_message_queue(const char *shm_name);

// Funzioni di gestione dei processi
int execute_Configchild_process(const char *shm_name);
int create_and_execute_atomo(int num, const char *shm_name, int msgid);
int create_and_execute_attivatore(const char *shm_name, int msgid);
int create_and_execute_alimentazione(int step);
void wait_for_Configchild_process(pid_t pid);

// Funzioni di inizializzazione
void init_master_shared_memory_and_semaphore(const char *shm_name, int *shm_fd, int *sem_id, SharedMemory *shared_memory);
void init_config_process(const char *shm_name, SharedMemory shared_memory, int shm_fd, int sem_id, int msgid);
void init_message_queue(const char *shm_name, int *msgid, SharedMemory shared_memory, int shm_fd, int sem_id);

// Funzioni di ciclo principale
void master_main_loop(int sem_id, SharedMemory shared_memory);

// Cleanup e gestione errori
void cleanup(SharedMemory shared_memory, int shm_fd, const char *shm_name, int sem_id, int msgid);
void handle_error(const char *msg, SharedMemory shared_memory, int shm_fd, const char *shm_name, int sem_id, int msgid);

#endif // MASTER_H
