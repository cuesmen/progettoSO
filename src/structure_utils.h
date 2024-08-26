#ifndef STRUCTURE_UTILS_H
#define STRUCTURE_UTILS_H

#define MEMSIZE 9
#define MSG_KEY 1234 

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
    int start_with_inibitore;
    int hard_kill_atoms;
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
    int *total_inibitore_energy;
    int *inibitore_attivo; 
} SharedMemory;


// Struttura per la coda di messaggi
struct msg_buffer {
    long msg_type;
    char msg_text[100];
};

struct atomo_msg_buffer  {
    long msg_type;
    int energia_ricevuta;
    int energia_da_ridurre;
};



#endif 