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
#include <sys/resource.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "../../structure_utils.h"
#include "../../semaphore_utils.h"
#include "../../log_utils.h"

typedef enum {
    TIMEOUT = 0,
    EXPLODE = 1,
    BLACKOUT = 2,
    MELTDOWN = 3,
    EXCEDED_PROCESSES = 4,
    PROGRAM_ERROR = 5,
} ExitCode;

// Funzioni per segnali
void handle_sigusr1(int sig);

// Funzioni IPC
int create_semaphore(key_t sem_key);
int create_and_open_shared_memory(const char *shm_name, int *shm_fd);
SharedMemory map_shared_memory(int shm_fd);
int create_message_queue(const char *shm_name);

// Funzioni di gestione dei processi
int execute_Configchild_process();
int create_and_execute_atomo(int num);
int create_and_execute_attivatore();
int create_and_execute_alimentazione(long step, int max_atoms);
void wait_for_Configchild_process(pid_t pid);

// Funzioni di inizializzazione
void init_master_shared_memory_and_semaphore();
void init_config_process();
void init_message_queue();

// Funzioni di ciclo principale
ExitCode master_main_loop();

// Cleanup e gestione errori
void cleanup();
void handle_error(const char *msg);
const char* get_exit_code_description(ExitCode code);

void set_termination_flag();

#endif // MASTER_H
