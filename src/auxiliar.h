#ifndef AUXILIAR_H
#define AUXILIAR_H

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct Mobile_Node {
    int num_pedidos;
    int intervalo_tempo;
    int mips;
    int max_tempo;

} MN;

typedef struct {
    char nome[50];
    int mips1;
    int mips2;

    pid_t pid;
    pthread_t vCPU1; // Usar em modo Normal
    pthread_t vCPU2; // Usar em modo High Performance
    pthread_mutex_t mutex; // Semaforo altera o numero de vCPUs em uso i.e alterna entre Normal e HP
} Edge_Server;

typedef struct shared_memory {
    int QUEUE_POS;
    int MAX_WAIT;
    int EDGE_SERVER_NUMBER;
    Edge_Server *servers;

    pid_t TM_pid;
    pid_t monitor_pid;
    pid_t maintenance_pid;

    sem_t *sem_manutencao; // semaforo usado para parar os Edge Servers 
    sem_t *sem_tarefas; // controlar as tarefas feitas pelos ES na MQ (2 ES nao fazerem a mesma tarefa)
    sem_t *sem_ficheiro; // nao haverem 2 processos a escreverem no log ao mesmo tempo

    int CPU_ativos;
    // funcao colocar da FIFO ira verificar se a capacidade da lista esta acima de 80%
    // funcao retirar da FIFO ira verificar se a capacidade da lista esta abaixo de 20%
} SM;

typedef struct no {
    // some data
    bool ocupado;
} no;

#define TAM_FIFO 100 // TODO: alterar tamanho da FIFO

typedef struct lista_ligada {
    no no[TAM_FIFO];
    int TAM;
} MQ;

void erro(char *msg);

void time_now(char * string);

void log_msg(char *msg, SM *shared_memory, int first_time);

void config(char *path, SM *shared_memory);

void createEdgeServers(char *path, SM *shared_memory);

void SIGTSTP_HANDLER(int signum);

void SIGINT_HANDLER(int signum);

void *function(void *t);

#endif