#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>
#include <sys/shm.h>

#include "auxiliar.h"

typedef struct {
    char nome[50];
    int mips1;
    int mips2;
}Edge_Server;

typedef struct shared_memory {
    int QUEUE_POS;
    int MAX_WAIT;
    int EDGE_SERVER_NUMBER;
    Edge_Server * servers;
}SM;

SM *shared_memory;
// pthread_mutex_t semaforo = PTHREAD_MUTEX_INITIALIZER;
// pthread_t monitor;
// pthread_cond_t condicao = PTHREAD_COND_INITIALIZER;

void config(char *path, SM *shared_memory);
void createEdgeServers(char *path);

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
    createEdgeServers(path);
    printf("%s\n",shared_memory->servers[0].nome);
    printf("%s\n",shared_memory->servers[1].nome);
    printf("%s\n",shared_memory->servers[2].nome);

    log_msg("O programa iniciou", 0);

    return 0;
}

// funcao que le o ficheiro de configuracao
void config(char *path, SM *shared_memory){ 
    FILE * fich = fopen(path, "r");
    assert(fich);

    char line[20];

    fscanf(fich, "%s", line);
    shared_memory->QUEUE_POS=atoi(line);

    fscanf(fich, "%s", line);
    shared_memory->MAX_WAIT = atoi(line);

    fscanf(fich, "%s", line);
    shared_memory->EDGE_SERVER_NUMBER = atoi(line);
    fclose(fich);
}

// funcao que cria os edge servers com base nos dados obtidos no ficheiro config
void createEdgeServers(char *path) {

    FILE * fich = fopen(path, "r");
    assert(fich);

    char line[20];
    const char sep[2] = ",";

    int num = 0;
    int i = 0;
    char *ptr;
    while (fscanf(fich, "%[^\n] ", line) != EOF) {
        if(num > 2){
            char *token;
            token = strtok(line, sep);
            int n = 0;
            while (token != NULL) {
                if (n == 0){
                    strcpy(shared_memory->servers[i].nome, token);
                    n++;
                }

                else if (n == 1){
                    int ret ;
                    ret = (int) strtol(token, &ptr, 10);
                    shared_memory->servers[i].mips1 = ret;
                    n++;
                }
                else if (n == 2){
                    int ret;
                    ret = (int) strtol(token, &ptr, 10);
                    shared_memory->servers[i].mips2 = ret;
                    n++;
                }

                token = strtok(NULL, sep);
            }
            i++;
        }
        num++;
    }
    fclose(fich);
}

