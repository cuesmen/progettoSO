#include "gui.h"

#define SHM_NAME "/wConfig"
#define SEM_KEY 1234

SharedMemory shared_memory;
int sem_id = -1;
GtkWidget *label_memory = NULL;
GtkWidget *label_processes = NULL;
GtkWidget *status_label = NULL;
GtkWidget *button = NULL;
GtkWidget *title_label = NULL;
GtkWidget *label_config = NULL;
GtkWidget *separator = NULL;
GtkWidget *signal_button_inibitore_on = NULL;
GtkWidget *signal_button_inibitore_off = NULL;
bool running = false;
bool ipc_resources_available = false;

// Variabili per tenere traccia dell'ultimo stato valido
int last_total_atoms = 0;
int last_total_attivatore = 0;
int last_total_splits = 0;
int last_total_wastes = 0;
int last_free_energy = 0;
int last_master_count = 0;
int last_atomo_count = 0;
int last_attivatore_count = 0;
int last_alimentazione_count = 0;
int last_inibitore_count = 0;
int last_total_inibitore_energy = 0;
int last_inibitore_attivo = 0;
int last_inibitore_wastes = 0;
Config last_config;

void init_shared_memory()
{
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        ipc_resources_available = false;
        return;
    }

    // Mappiamo la memoria condivisa nel nostro spazio di indirizzamento
    void *shared_area = mmap(NULL, sizeof(Config) + sizeof(int) * MEMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_area == MAP_FAILED)
    {
        perror("mmap");
        close(shm_fd);
        ipc_resources_available = false;
        return;
    }

    // Impostiamo i puntatori della memoria condivisa per accedere ai dati
    shared_memory.shared_config = (Config *)shared_area;
    shared_memory.shared_free_energy = (int *)((char *)shared_area + sizeof(Config));
    shared_memory.total_atoms_counter = (int *)((char *)shared_area + sizeof(Config) + sizeof(int));
    shared_memory.total_atoms = (int *)((char *)shared_area + sizeof(Config) + 2 * sizeof(int));
    shared_memory.toEnd = (int *)((char *)shared_area + sizeof(Config) + 3 * sizeof(int));
    shared_memory.total_wastes = (int *)((char *)shared_area + sizeof(Config) + 4 * sizeof(int));
    shared_memory.total_attivatore = (int *)((char *)shared_area + sizeof(Config) + 5 * sizeof(int));
    shared_memory.total_splits = (int *)((char *)shared_area + sizeof(Config) + 6 * sizeof(int));
    shared_memory.total_inibitore_energy = (int *)((char *)shared_area + sizeof(Config) + 7 * sizeof(int));
    shared_memory.inibitore_attivo = (int *)((char *)shared_area + sizeof(Config) + 8 * sizeof(int));
    shared_memory.total_wastes_by_inibitore = (int *)((char *)shared_area + sizeof(Config) + 9 * sizeof(int));

    // Salviamo la configurazione per usarla più tardi
    last_config = *(shared_memory.shared_config);

    close(shm_fd);
    ipc_resources_available = true;
}

void init_semaphore()
{
    key_t sem_key = ftok(SHM_NAME, 'S');
    sem_id = semget(sem_key, 1, IPC_CREAT | 0666);
    if (sem_id == -1)
    {
        perror("semget");
        ipc_resources_available = false;
        return;
    }

    semctl(sem_id, 0, SETVAL, 1);
    ipc_resources_available = true;
}

int count_processes(const char *process_name)
{
    char command[256];
    snprintf(command, sizeof(command), "pgrep -c %s", process_name);
    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("popen");
        return -1;
    }

    int count;
    if (fscanf(fp, "%d", &count) != 1)
    {
        perror("fscanf");
        count = -1;
    }
    pclose(fp);
    return count;
}

void update_labels()
{
    // Otteniamo il conteggio dei processi attivi
    int master_count = count_processes("master");
    int atomo_count = count_processes("atomo");
    int attivatore_count = count_processes("attivatore");
    int inibitore_count = count_processes("inibitore");
    int alimentazione_count = count_processes("alimentazione");

    // Aggiorniamo le variabili per i conteggi
    last_master_count = master_count;
    last_atomo_count = atomo_count;
    last_attivatore_count = attivatore_count;
    last_alimentazione_count = alimentazione_count;
    last_inibitore_count = inibitore_count;

    // Creiamo il testo da visualizzare nella label dei processi
    char buffer_processes[256];
    snprintf(buffer_processes, sizeof(buffer_processes),
             "Conteggi processi:\nMaster: %d\nAtomo: %d\nAttivatore: %d\nInibitore: %d\nAlimentazione: %d",
             last_master_count, last_atomo_count, last_attivatore_count, last_inibitore_count, last_alimentazione_count);
    gtk_label_set_text(GTK_LABEL(label_processes), buffer_processes);

    if (!ipc_resources_available)
    {
        gtk_label_set_text(GTK_LABEL(status_label), "Risorse IPC non disponibili");

        // Se non riusciamo ad accedere alle risorse IPC, mostriamo gli ultimi valori validi
        char buffer_memory[256];
        snprintf(buffer_memory, sizeof(buffer_memory),
                 "Energia liberata: %d\nAtomi totali: %d\nAttivazioni: %d\nScissioni: %d\nScorie: %d\nEnergia inibitore: %d\nInibitore attivo: %d",
                 last_free_energy, last_total_atoms, last_total_attivatore, last_total_splits, last_total_wastes, last_total_inibitore_energy, last_inibitore_attivo);

        gtk_label_set_text(GTK_LABEL(label_memory), buffer_memory);
        return;
    }

    if (sem_id == -1)
    {
        gtk_label_set_text(GTK_LABEL(status_label), "Semaforo non disponibile");
        gtk_label_set_text(GTK_LABEL(label_memory), "Memoria condivisa non sincronizzata.");
        return;
    }

    if (semctl(sem_id, 0, GETVAL) == -1)
    {
        gtk_label_set_text(GTK_LABEL(status_label), "Errore: Semaforo non esistente o non accessibile");
        return;
    }

    semaphore_p(sem_id); // Acquisiamo il semaforo per sincronizzare l'accesso

    // Aggiorniamo i valori leggendo dalla memoria condivisa
    last_total_atoms = *(shared_memory.total_atoms);
    last_total_attivatore = *(shared_memory.total_attivatore);
    last_total_splits = *(shared_memory.total_splits);
    last_total_wastes = *(shared_memory.total_wastes);
    last_free_energy = *(shared_memory.shared_free_energy);
    last_total_inibitore_energy = *(shared_memory.total_inibitore_energy);
    last_inibitore_attivo = *(shared_memory.inibitore_attivo);
    last_inibitore_wastes = *(shared_memory.total_wastes_by_inibitore);

    semaphore_v(sem_id); // Rilasciamo il semaforo

    // Aggiorniamo l'etichetta con i nuovi dati
    char buffer_memory[256];
    snprintf(buffer_memory, sizeof(buffer_memory),
             "Energia liberata: %d\nAtomi totali: %d\nAttivazioni: %d\nScissioni: %d\nScorie: %d\nEnergia inibitore: %d\nInibitore attivo: %d\nScorie inibitore: %d",
             last_free_energy, last_total_atoms, last_total_attivatore, last_total_splits, last_total_wastes, last_total_inibitore_energy, last_inibitore_attivo, last_inibitore_wastes);
    gtk_label_set_text(GTK_LABEL(label_memory), buffer_memory);

    gtk_label_set_text(GTK_LABEL(status_label), ""); // Puliamo il messaggio di stato
}

gboolean update_labels_periodically(gpointer data)
{
    if (running)
    {
        update_labels();
        return TRUE; // Continua a eseguire
    }
    return FALSE; // Interrompi l'esecuzione
}

void start_periodic_update()
{
    running = true;
    gtk_button_set_label(GTK_BUTTON(button), "Stop");
    g_timeout_add_seconds(1, update_labels_periodically, NULL); // Aggiorniamo ogni secondo
}

void on_button_clicked(GtkWidget *widget, gpointer data)
{
    if (!running)
    {
        start_periodic_update();
    }
    else
    {
        running = false;
        gtk_button_set_label(GTK_BUTTON(button), "Start");
    }
}

pid_t find_master_pid()
{
    char command[] = "pgrep master";
    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("popen");
        return -1;
    }

    pid_t pid;
    if (fscanf(fp, "%d", &pid) != 1)
    {
        pclose(fp);
        return -1;
    }

    pclose(fp);
    return pid;
}

void on_signal_button_inibitore_on_clicked(GtkWidget *widget, gpointer data)
{
    pid_t master_pid = find_master_pid();
    if (master_pid > 0)
    {
        kill(master_pid, SIGRTMIN); // Inviamo il segnale per attivare l'inibitore
    }
    else
    {
        g_print("Errore: processo master non trovato.\n");
    }
}

void on_signal_button_inibitore_off_clicked(GtkWidget *widget, gpointer data)
{
    pid_t master_pid = find_master_pid();
    if (master_pid > 0)
    {
        kill(master_pid, SIGRTMIN + 1); // Inviamo il segnale per disattivare l'inibitore
    }
    else
    {
        g_print("Errore: processo master non trovato.\n");
    }
}

gboolean start_update_after_delay(gpointer data)
{
    start_periodic_update();
    return FALSE; // Interrompi l'esecuzione di questo timeout
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    init_shared_memory();
    init_semaphore();

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Creiamo le label e i pulsanti
    title_label = gtk_label_new("Master Viewer");
    label_processes = gtk_label_new("Conteggi processi:\nMaster: 0\nAtomo: 0\nAttivatore: 0\nAlimentazione: 0");
    label_memory = gtk_label_new("Premi il pulsante per avviare il conteggio.");
    label_config = gtk_label_new("Configurazione:\nAtomi iniziali: 0\nAtomi max: 0\nDebug: 0\nSleep atomo: 0.00\nStep attivatore: 0\nMin atomi: 0\nStep alimentazione: 0\nDurata sim: 0\nThreshold esplosione energia: 0\nDomanda energia: 0");
    status_label = gtk_label_new("");
    button = gtk_button_new_with_label("Start");

    signal_button_inibitore_on = gtk_button_new_with_label("Inibitore ON");
    signal_button_inibitore_off = gtk_button_new_with_label("Inibitore OFF");

    separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    // Definiamo il CSS per gli elementi
    const gchar *css_data = "label {\n"
                            "   font-size: 16px;\n"
                            "   margin: 5px;\n"
                            "}\n"
                            "button {\n"
                            "   font-size: 16px;\n"
                            "   padding: 10px;\n"
                            "}\n"
                            "grid {\n"
                            "   padding: 10px;\n"
                            "}\n";
    gssize css_length = (gssize)strlen(css_data);

    GtkCssProvider *css_provider = gtk_css_provider_new();
    gboolean success = gtk_css_provider_load_from_data(css_provider, css_data, css_length, NULL);

    if (!success)
    {
        g_warning("Failed to load CSS data.");
    }

    // Applichiamo il CSS agli elementi
    GtkStyleContext *context;

    context = gtk_widget_get_style_context(label_processes);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    context = gtk_widget_get_style_context(label_memory);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    context = gtk_widget_get_style_context(label_config);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    context = gtk_widget_get_style_context(button);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    context = gtk_widget_get_style_context(signal_button_inibitore_on);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    context = gtk_widget_get_style_context(signal_button_inibitore_off);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_widget_set_size_request(label_processes, 350, -1);
    gtk_widget_set_size_request(label_memory, 350, -1);
    gtk_widget_set_size_request(label_config, 350, -1);
    gtk_widget_set_size_request(button, 150, 50);
    gtk_widget_set_size_request(signal_button_inibitore_on, 150, 50);
    gtk_widget_set_size_request(signal_button_inibitore_off, 150, 50);

    // Organizziamo il layout nella griglia
    gtk_grid_attach(GTK_GRID(grid), title_label, 0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), label_config, 0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), separator, 0, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), label_processes, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label_memory, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 4, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), signal_button_inibitore_on, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), signal_button_inibitore_off, 1, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), status_label, 0, 6, 2, 1);

    gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(label_config, GTK_ALIGN_FILL);
    gtk_widget_set_halign(separator, GTK_ALIGN_FILL);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), NULL);

    // Connettiamo i segnali ai pulsanti
    g_signal_connect(signal_button_inibitore_on, "clicked", G_CALLBACK(on_signal_button_inibitore_on_clicked), NULL);
    g_signal_connect(signal_button_inibitore_off, "clicked", G_CALLBACK(on_signal_button_inibitore_off_clicked), NULL);

    // Facciamo partire l'aggiornamento con un piccolo ritardo
    g_timeout_add_seconds(1, start_update_after_delay, NULL);

    // Aggiorniamo la configurazione iniziale se le risorse IPC sono disponibili
    if (ipc_resources_available)
    {
        char buffer_config[512];
        snprintf(buffer_config, sizeof(buffer_config),
                 "Configurazione:\nAtomi iniziali: %d\nAtomi max: %d\nDebug: %d\nSleep atomo: %.2f\nStep attivatore: %d\nMin atomi: %d\nStep alimentazione: %ld\nDurata sim: %d\nThreshold esplosione energia: %d\nDomanda energia: %d\nStart with inibitore: %d\nHard kill atoms: %d",
                 last_config.n_atomi_init, last_config.n_atom_max, last_config.debug, last_config.atom_sleep,
                 last_config.step_attivatore, last_config.min_n_atomico, last_config.step_alimentazione,
                 last_config.sim_duration, last_config.energy_explode_threshold, last_config.energy_demand,
                 last_config.start_with_inibitore, last_config.hard_kill_atoms);

        gtk_label_set_text(GTK_LABEL(label_config), buffer_config);
    }

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
