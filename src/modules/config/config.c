#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#include "config.h"


Config *globalConfig;

static int handler(void* user, const char* section, const char* name, const char* value) {
    Config* pconfig = (Config*)user;

    if (strcmp(section, "ATOMI") == 0) {
        if (strcmp(name, "N_ATOMI_INIT") == 0) {
            pconfig->n_atomi_init = atoi(value);
        }
        if (strcmp(name, "N_ATOM_MAX") == 0) {
            pconfig->n_atom_max = atoi(value);
        }
         if (strcmp(name, "MIN_N_ATOMICO") == 0) {
            pconfig->min_n_atomico = atoi(value);
        }
    }
    if (strcmp(section, "ATTIVATORE") == 0) {
        if (strcmp(name, "STEP_ATTIVATORE") == 0) {
            pconfig->step_attivatore = atoi(value);
        }
    }
    if (strcmp(section, "ALIMENTAZIONE") == 0) {
        if (strcmp(name, "STEP_ALIMENTAZIONE") == 0) {
            pconfig->step_alimentazione = atoi(value);
        }
    }
    if (strcmp(section, "GENERAL") == 0) {
        if (strcmp(name, "debug") == 0) {
            pconfig->debug = atoi(value);
        }
        if (strcmp(name, "atom_sleep") == 0) {
            pconfig->atom_sleep = atof(value);
        }
    }

    return 1;  
}

int loadConfig() {
    const char *filename = "config.ini";
    if (ini_parse(filename, handler, globalConfig) < 0) {
        printf("Errore nel caricamento del file '%s'\n", filename);
        fflush(stdout);
        return 1;
    }
    return 0;
}

void printConfig() {
    printf("Numero di atomi iniziali: %d\n", globalConfig->n_atomi_init);
    printf("Numero atom max: %d\n", globalConfig->n_atom_max);
    printf("Numero minimo di un atomo: %d\n", globalConfig->min_n_atomico);
    printf("Debug mode?: %d\n", globalConfig->debug);
    printf("Atom sleep: %.1f\n", globalConfig->atom_sleep);
    printf("Step attivatore: %d\n", globalConfig->step_attivatore);
    printf("Step alimentazione: %d\n", globalConfig->step_alimentazione);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Utilizzo: %s <nome_shm>\n", argv[0]);
        fflush(stdout);
        return 1;
    }

    const char *shm_name = argv[1];
    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Mappa la memoria condivisa
    globalConfig = mmap(NULL, sizeof(Config), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (globalConfig == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    if (loadConfig() == 0) {
        printConfig();
    } else {
        return 1; // Errore durante il caricamento della configurazione
    }

    // Pulizia
    munmap(globalConfig, sizeof(Config));
    close(shm_fd);

    return 0; // Successo
}
