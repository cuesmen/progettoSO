#ifndef ATOMO_H
#define ATOMO_H


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "../master/master.h"
#include "../config/config.h"
#include "../semaphore_utils.h"

int map_atomo_shared_memory(const char *shm_name);
void split_and_create_new_atomo();
int get_semaphore_id(const char *shm_name);
void printDebug(const char *format, ...);

#endif // ATOMO_H
