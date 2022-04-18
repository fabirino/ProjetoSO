// ./offload_simulator configfile.txt

#include "auxiliar.h"

SM *shared_memory;
// pthread_mutex_t semaforo = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t condicao = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]) {

    if (argc != 2) {
        erro("Parametros errados. Exemplo:\noffload_simulator <config_file>\n");
    }

    // Create shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0766);

    // Attach shared memory
    shared_memory = (SM *)shmat(shmid, NULL, 0);

    // Inicializar semaforos;
    sem_unlink("SEM_MANUTENCAO");
    sem_unlink("SEM_TAREFAS");
    sem_unlink("SEM_FICHEIRO");
    shared_memory->sem_manutencao = sem_open("SEM_MANUTENCAO", O_CREAT | O_EXCL, 0700, 1);
    shared_memory->sem_tarefas = sem_open("SEM_TAREFAS", O_CREAT | O_EXCL, 0700, 1);
    shared_memory->sem_ficheiro = sem_open("SEM_FICHEIRO", O_CREAT | O_EXCL, 0700, 1);

    // Create Named Pipe
    // TODO:

    // Create Message QUEUE
    // TODO:

    // Read config file
    char path[20];
    strcpy(path, argv[1]);

    config(path, shared_memory);
    shared_memory->servers = (Edge_Server *)malloc(sizeof(Edge_Server) * shared_memory->EDGE_SERVER_NUMBER);
    createEdgeServers(path, shared_memory);
    printf("%s\n", shared_memory->servers[0].nome);
    printf("%s\n", shared_memory->servers[1].nome);
    printf("%s\n", shared_memory->servers[2].nome);

    // #=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

    log_msg("O programa iniciou", shared_memory, 1);

    // Catch Signals
    signal(SIGTSTP, SIGTSTP_HANDLER);
    signal(SIGINT, SIGINT_HANDLER);

    // Task Manager ========================================================================
    if ((shared_memory->TM_pid = fork()) == 0) {
        // DEBUG:
        log_msg("O processo Task Manager comecou", shared_memory, 0);

        task_menager(shared_memory);

        while (wait(NULL) > 0)
            ;

        exit(0);
    }

    // Monitor =============================================================================
    if ((shared_memory->monitor_pid = fork()) == 0) {
        // DEBUG:
        log_msg("O processo Monitor comecou", shared_memory, 0);

        exit(0);
    }

    // Maintenance Manager =================================================================
    if ((shared_memory->maintenance_pid = fork()) == 0) {
        // DEBUG:
        log_msg("O processo Maintenance Manager comecou", shared_memory, 0);

        exit(0);
    }

    while (wait(NULL) > 0)
        ;

    return 0;
}
