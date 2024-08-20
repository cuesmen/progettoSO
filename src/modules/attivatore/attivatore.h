#ifndef ATTIVATORE_H
#define ATTIVATORE_H

#include "../../structure_utils.h"
#include "../../semaphore_utils.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <stdarg.h>
#include <errno.h> 
#include <signal.h>

// Funzioni IPC
int get_semaphore_id(const char *shm_name);
int map_attivatore_shared_memory(const char *shm_name, SharedMemory **shared_memory);

// Funzioni di invio messaggi
int send_message_to_atoms(int msgid, long msg_type, const char *msg_text);

// Funzioni di inizializzazione
void init_shared_memory_and_semaphore(const char *shm_name, int *sem_id, int msgid);
void init_attivatore(int sem_id) ;

// Funzioni di ciclo principale
void attivatore_main_loop(int sem_id, int msgid, int step_attivatore);

// Cleanup e gestione errori
void cleanup_ipc_resources(int msgid, SharedMemory *shared_memory);
void handle_Attivatoreerror(const char *msg, int msgid, SharedMemory *shared_memory);



#endif
