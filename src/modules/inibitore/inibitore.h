#ifndef INIBITORE_H
#define INIBITORE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <errno.h>
#include <time.h> 

#include "../../log_utils.h"
#include "../../structure_utils.h"
#include "../../semaphore_utils.h"

// Costanti di configurazione
#define PERCENTUALE_ASSORBIMENTO 0.2
#define INTERVALLO_CONTROLLO 1


// Funzioni principali del processo inibitore
void handle_signal(int sig);            // Gestore dei segnali per attivare/disattivare l'inibitore
void setup_signal_handlers();           // Imposta i gestori dei segnali
int calcola_energia_da_ridurre(int energia_ricevuta);  // Calcola la quantità di energia da assorbire
void inibitore_main_loop(int msgid);             // Loop principale del processo inibitore

#endif // INIBITORE_H
