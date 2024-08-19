#ifndef ATTIVATORE_H
#define ATTIVATORE_H

#include "../master/master.h"
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
#include <semaphore.h>

struct msg_buffer {
    long msg_type;
    char msg_text[100];
};

int get_step_attivatore(const char *shm_name);
int send_message_to_atoms(int msgid, long msg_type, const char *msg_text);
void cleanup_ipc_resources(int msgid);


#endif
