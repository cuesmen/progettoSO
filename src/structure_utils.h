#ifndef STRUCTURE_UTILS_H
#define STRUCTURE_UTILS_H

// Struttura che contiene i parametri di configurazione
typedef struct {
    int n_atomi_init;  
    int n_atom_max;
    int debug;
    float atom_sleep;
    int step_attivatore;
    int min_n_atomico;
    int step_alimentazione;
} Config;


// Struttura che contiene la memoria 
typedef struct {
    Config *shared_config;
    int *shared_free_energy;
    int *total_atoms_counter;
    int *total_atoms;
} SharedMemory;


// Struttura per la coda di messaggi
struct msg_buffer {
    long msg_type;
    char msg_text[100];
};


#endif 