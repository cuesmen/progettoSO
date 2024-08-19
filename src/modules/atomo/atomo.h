#ifndef ATOMO_H
#define ATOMO_H


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/mman.h>

#include <fcntl.h>

#include "../master/master.h"
#include "../config/config.h"
#include "../semaphore_utils.h"

struct msg_buffer {
    long msg_type;
    char msg_text[100];
};

int map_atomo_shared_memory(const char *shm_name);
void split_and_create_new_atomo(const char *shm_name, int sem_id);
int get_semaphore_id(const char *shm_name);
void printDebug(const char *format, ...);
void freeEnergy(int n1, int n2, int sem_id);
int getMax(int n1, int n2);
void cleanup_ipc_resources(const char *shm_name, int sem_id, int msgid);

#endif // ATOMO_H
