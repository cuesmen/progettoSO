#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../libs/ini.h"

#ifndef CONFIG_H
#define CONFIG_H

// Struttura che contiene i parametri di configurazione
typedef struct {
    int n_atomi_init;  // Variabile che corrisponde a N_ATOMI_INIT nella sezione [GENERAL]
} Config;

// Dichiarazione della variabile globale
extern Config globalConfig;

// Funzione per caricare la configurazione dal file .ini
int loadConfig();

// Funzione per stampare a video la configurazione
void printConfig();

#endif // CONFIG_H

