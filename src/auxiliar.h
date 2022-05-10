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
#define READ 0
#define WRITE 1

//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#
typedef struct {
    // dados
    int idTarefa;
    int num_instrucoes;
    int max_tempo;
    struct tm tempo_chegada;
} Task;

typedef struct {
    int ES_num;
    int mips_vcpu;
    int num_instrucoes;
    char nome_server[50];
    int n_vcpu;
    int idTarefa;
} argumentos;

typedef struct {
    Task tarefa;
    bool ocupado;
    int prioridade;
    int mens_seguinte;
} no_fila;

typedef struct {
    no_fila *nos;
    int n_tarefas;
    int entrada_lista;
} base;

bool colocar(base *pf, Task tarefa, int prioridade);

bool retirar(base *pf, Task *ptarefa);

void inicializar(base *pf);

void reoorganizar(base *pf, time_t tempo);

//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

int MQid;

typedef struct {
    /*Message Type*/
    long priority;
    /*Payload*/
    int temp_man;
} priority_msg;

typedef struct {
    char nome[50];
    int mips1;
    int mips2;

    pid_t pid;
    pthread_t vCPU[2];     // Usar em modo High Performance
    pthread_mutex_t mutex; // Semaforo altera o numero de vCPUs em uso i.e alterna entre Normal e HP

    int em_manutencao; // Modo Stopped
    int manutencao;
    int tarefas_executadas;
    int manutencoes;

    int es_ativo;
    int cpu_ativo[2];

    int fd[2];

} Edge_Server;

typedef struct shared_memory {
    int QUEUE_POS;
    int MAX_WAIT;
    int EDGE_SERVER_NUMBER;

    int n_tarefas;
    int tempo_medio;

    pid_t TM_pid;
    pid_t monitor_pid;
    pid_t maintenance_pid;

    sem_t *sem_manutencao; // Semaforo usado para parar os Edge Servers
    sem_t *sem_tarefas;    // controlar as tarefas feitas pelos ES na MQ (2 ES nao fazerem a mesma tarefa)
    sem_t *sem_ficheiro;   // Semaforo para escrever no ficheiro log
    sem_t *sem_SM;         // Semaforo para ler e escrever da Shared Memory
    sem_t *sem_servers;    // Semaforo para esperar para que
    sem_t *sem_performace; // 
    sem_t *sem_fila; // 

    pthread_mutex_t mutex_dispatcher; // semaforo para as threads
    pthread_cond_t cond_dispatcher;   // variavel de condicao que muda de Normal para HP
    pthread_mutex_t mutex_manutencao; 
    pthread_cond_t cond_manutencao; 
    pthread_mutex_t mutex_monitor; 
    pthread_cond_t cond_monitor;     

    int Num_es_ativos; // numero de edge servers ativos!!

    int mode_cpu;
    int tarefas_descartadas;

} SM;

SM *shared_memory;

Edge_Server *servers;

pthread_mutexattr_t mattr;
pthread_condattr_t cattr;

int shmid;
int shmserversid;

void erro(char *msg);

void time_now(char *string);

void log_msg(char *msg, int first_time);

void config(char *path);

void createEdgeServers(char *path);

void SIGTSTP_HANDLER(int signum);

void SIGINT_HANDLER(int signum);

void task_manager();

void server(int i);

void *vCPU_routine(void *t);

#endif