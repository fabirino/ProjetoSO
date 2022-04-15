
#include "auxiliar.h"

SM *shared_memory;
// pthread_mutex_t semaforo = PTHREAD_MUTEX_INITIALIZER;
// pthread_t monitor;
// pthread_cond_t condicao = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]) {
    
    if (argc !=2){
        erro("Parametros errados. Exemplo:\noffload_simulator <config_file>\n");
    }

    // Create shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0766);

    // Attach shared memory
    shared_memory = (SM *)shmat(shmid, NULL, 0);

    char path[20];
    strcpy(path, argv[1]);
    
    config(path, shared_memory);
    shared_memory->servers = (Edge_Server*)malloc(sizeof(Edge_Server) * shared_memory->EDGE_SERVER_NUMBER);
    createEdgeServers(path, shared_memory);
    printf("%s\n",shared_memory->servers[0].nome);
    printf("%s\n",shared_memory->servers[1].nome);
    printf("%s\n",shared_memory->servers[2].nome);

    log_msg("O programa iniciou", 0);

    return 0;
}


