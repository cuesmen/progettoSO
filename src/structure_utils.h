#ifndef STRUCTURE_UTILS_H
#define STRUCTURE_UTILS_H

#define MEMSIZE 7

// Struttura che contiene i parametri di configurazione
typedef struct {
    int n_atomi_init;  
    int n_atom_max;
    int debug;
    float atom_sleep;
    int step_attivatore;
    int min_n_atomico;
    long step_alimentazione;
    int sim_duration;
    int energy_explode_threshold;
    int energy_demand;
} Config;


// Struttura che contiene la memoria 
typedef struct {
    Config *shared_config;
    int *shared_free_energy;
    int *total_atoms_counter;
    int *total_atoms;
    int *toEnd;
    int *total_wastes;
    int *total_attivatore;
    int *total_splits;
} SharedMemory;


// Struttura per la coda di messaggi
struct msg_buffer {
    long msg_type;
    char msg_text[100];
};


#endif 