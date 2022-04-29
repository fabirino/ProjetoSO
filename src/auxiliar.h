// Eduardo Figueiredo 2020213717
// Fábio Santos 2020212310

#ifndef AUXILIAR_H
#define AUXILIAR_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE 1024
#define PIPE_NAME "TASK_PIPE"

int MQid;

typedef struct {
    /* Message type */
    long priority;
    /* Payload */
    int msg_number; // TODO: usar este numero para saber quantas tarefas estao por terminar quando o programa acabar
    // dados
    int idTarefa;
    int num_pedidos;
    int max_tempo;
} priority_msg;

//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#
typedef struct {
    // dados
    int idTarefa;
    int num_pedidos;
    int max_tempo;
} Task;

typedef struct {
    Task tarefa;
    bool ocupado;
    int prioridade;
    int mens_seguinte;
} no_fila;

typedef struct {
    no_fila * nos; // dados
    int tam;
    int entrada_lista;
} base;

bool colocar(base *pf, Task tarefa, int prioridade);

bool retirar(base *pf, Task *ptarefa);

void inicializar(base *pf,int tamanho);

//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

typedef struct {
    char nome[50];
    int mips1;
    int mips2;

    pid_t pid;
    pthread_t vCPU[2];     // Usar em modo High Performance
    pthread_mutex_t mutex; // Semaforo altera o numero de vCPUs em uso i.e alterna entre Normal e HP

    int em_manutencao; // Modo Stopped
    int tarefas_executadas;
    int manutencoes;

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
    sem_t *sem_tarefas;    // controlar as tarefas feitas pelos ES na MQ (2 ES nao fazerem a mesma tarefa)
    sem_t *sem_ficheiro;   // nao haverem 2 processos a escreverem no log ao mesmo tempo
    sem_t *sem_SM;         // Semaforo para ler e escrever da Shared Memory

    int CPU_ativos;
    int tarefas_descartadas;

} SM;

typedef struct {
    int capacidade_vcpu;
    char nome_server[50];
    int n_vcpu;
} argumentos;

SM *shared_memory;

void erro(char *msg);

void time_now(char *string);

void log_msg(char *msg, int first_time);

void config(char *path);

void createEdgeServers(char *path);

void SIGTSTP_HANDLER(int signum);

void SIGINT_HANDLER(int signum);

void task_menager();

void Server(int i);

void *ES_routine(void *t);

#endif