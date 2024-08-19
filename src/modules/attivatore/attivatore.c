#include "attivatore.h"
// Funzione per ottenere il valore di step_attivatore dalla memoria condivisa
int get_step_attivatore(const char *shm_name)
{
    int shm_fd;
    shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        return -1;
    }

    // Mappiamo solo la porzione necessaria per ottenere il valore di step_attivatore
    void *shared_area = mmap(NULL, sizeof(Config), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_area == MAP_FAILED)
    {
        perror("mmap");
        close(shm_fd);
        return -1;
    }

    // Accedi al valore di step_attivatore
    Config *config = (Config *)shared_area;
    int step_attivatore = config->step_attivatore;

    // Cleanup
    munmap(shared_area, sizeof(Config));
    close(shm_fd);

    return step_attivatore;
}

// Funzione per inviare un messaggio alla coda di messaggi
int send_message_to_atoms(int msgid, long msg_type, const char *msg_text)
{
    struct msg_buffer message;
    message.msg_type = msg_type;
    strncpy(message.msg_text, msg_text, sizeof(message.msg_text) - 1);
    message.msg_text[sizeof(message.msg_text) - 1] = '\0'; // Assicurati che sia null-terminated

    if (msgsnd(msgid, &message, sizeof(message.msg_text), 0) == -1)
    {
        perror("msgsnd");
        return 1;
    }

    return 0;
}

void cleanup_ipc_resources(int msgid)
{
    if (msgctl(msgid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl IPC_RMID");
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <shared_memory_name> <msgid>\n", argv[0]);
        return 1;
    }

    const char *shm_name = argv[1];
    int msgid = atoi(argv[2]);

    int step_attivatore = get_step_attivatore(shm_name);
    if (step_attivatore == -1)
    {
        return 1;
    }

    printf("Attivatore avviato con shared_memory_name: %s\n", shm_name);
    printf("Valore di step_attivatore: %d\n", step_attivatore);

    sleep(1);

    while (1)
    {
        sleep(step_attivatore);
        srand(time(NULL));
        int r = rand() % 5 + 1;

        for (int i = 0; i < r; i++)
        {
            if (send_message_to_atoms(msgid, 1, "split") != 0)
            {
                printf("Errore nell'invio del messaggio agli atomi.\n");
                break;
            }
        }
    }


    // cleanup_ipc_resources(msgid);

    return 0;
}
