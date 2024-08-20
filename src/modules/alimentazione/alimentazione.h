#ifndef ALIMENTAZIONE_H
#define ALIMENTAZIONE_H

#include <stdio.h>
#include <stdlib.h>  
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

void handle_signal(int);
void create_atomo_from_alimentazione(int num, const char *shm_name, int msgid);

#endif 