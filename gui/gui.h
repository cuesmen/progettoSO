#ifndef MASTER_VIEWER_H
#define MASTER_VIEWER_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdbool.h>
#include "../src/semaphore_utils.h"
#include "../src/structure_utils.h"

// Definizioni per la memoria condivisa e il semaforo
#define SHM_NAME "/wConfig"
#define SEM_KEY 1234

// Dichiarazione delle variabili globali
extern SharedMemory shared_memory;
extern int sem_id;
extern GtkWidget *label_memory;
extern GtkWidget *label_processes;
extern GtkWidget *status_label;
extern GtkWidget *button;
extern GtkWidget *title_label;
extern GtkWidget *label_config;
extern GtkWidget *separator;
extern GtkWidget *signal_button_inibitore_on;
extern GtkWidget *signal_button_inibitore_off;
extern bool running;
extern bool ipc_resources_available;

// Variabili per tenere traccia dell'ultimo stato valido
extern int last_total_atoms;
extern int last_total_attivatore;
extern int last_total_splits;
extern int last_total_wastes;
extern int last_free_energy;
extern int last_master_count;
extern int last_atomo_count;
extern int last_attivatore_count;
extern int last_alimentazione_count;
extern int last_inibitore_count;
extern int last_total_inibitore_energy;
extern int last_inibitore_attivo;
extern int last_inibitore_wastes;
extern Config last_config;

// Dichiarazione delle funzioni
void init_shared_memory();
void init_semaphore();
int count_processes(const char *process_name);
void update_labels();
gboolean update_labels_periodically(gpointer data);
void start_periodic_update();
void on_button_clicked(GtkWidget *widget, gpointer data);
pid_t find_master_pid();
void on_signal_button_inibitore_on_clicked(GtkWidget *widget, gpointer data);
void on_signal_button_inibitore_off_clicked(GtkWidget *widget, gpointer data);
gboolean start_update_after_delay(gpointer data);

#endif 
