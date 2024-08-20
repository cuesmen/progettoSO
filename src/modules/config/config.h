#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../../../libs/ini.h"
#include "../../structure_utils.h"


Config *globalConfig;

// Funzione per caricare la configurazione dal file .ini
int loadConfig();

// Funzione per stampare a video la configurazione
void printConfig();

#endif // CONFIG_H
