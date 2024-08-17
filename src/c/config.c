#include "config.h"
#include "ini.h"

// Definizione della variabile globale
Config globalConfig;

// Funzione handler per inih
static int handler(void* user, const char* section, const char* name, const char* value) {
    Config* pconfig = (Config*)user;

    // Verifica se siamo nella sezione [GENERAL]
    if (strcmp(section, "GENERAL") == 0) {
        if (strcmp(name, "N_ATOMI_INIT") == 0) {
            pconfig->n_atomi_init = atoi(value);
        }
    }

    return 1;  
}

// Funzione per caricare la configurazione dal file .ini
int loadConfig() {
    const char *filename = "config.ini";
    if (ini_parse(filename, handler, &globalConfig) < 0) {
        printf("Errore nel caricamento del file '%s'\n", filename);
        return 1;
    }
    return 0;
}

void printConfig() {
    printf("Numero di atomi iniziali: %d\n", globalConfig.n_atomi_init);
}
