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

void reoorganizar(base *pf, struct timeval tempo);

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

// void *p_dispatcher(void *lista) { // distribuição das tarefas

//     Task received_msg; // DEBUG:apenas testes!!!! IR PARA OS EDGE SERVERS !!!

//     while (1) {
//         int count = 0;
//         sem_wait(shared_memory->sem_SM);
//         for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
//             if (servers[i].em_manutencao == 0) {
//                 if (servers[i].cpu_ativo[0] == 0)
//                     count++;
//                 if (servers[i].cpu_ativo[1] == 0)
//                     count++;
//             }
//         }
//         sem_post(shared_memory->sem_SM);

//         // Verificar os ES que estao livres
//         sem_wait(shared_memory->sem_SM);
//         sem_wait(shared_memory->sem_performace);
//         sem_wait(shared_memory->sem_fila);
//         pthread_mutex_lock(&shared_memory->mutex_dispatcher);
//         while (((shared_memory->mode_cpu == 1) && (shared_memory->Num_es_ativos >= shared_memory->EDGE_SERVER_NUMBER)) || (shared_memory->n_tarefas == 0) || ((shared_memory->mode_cpu == 2) && (count <= 0))) {
//             printf("DEBUG: while, %d | %d | %d | %d | count: %d | n_tarefas: %d\n", shared_memory->Num_es_ativos, servers[0].es_ativo, servers[1].es_ativo, servers[2].es_ativo, count, shared_memory->n_tarefas);
//             sem_post(shared_memory->sem_performace);
//             sem_post(shared_memory->sem_SM);
//             sem_post(shared_memory->sem_fila);
//             pthread_cond_wait(&shared_memory->cond_dispatcher, &shared_memory->mutex_dispatcher);
//             sem_wait(shared_memory->sem_SM);
//             for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
//                 if (servers[i].em_manutencao == 0) {
//                     if (servers[i].cpu_ativo[0] == 0)
//                         count++;
//                     if (servers[i].cpu_ativo[1] == 0)
//                         count++;
//                 }
//             }
//             sem_post(shared_memory->sem_SM);
//         }
//         retirar(&MQ, &received_msg);
//         sem_post(shared_memory->sem_performace);
//         sem_post(shared_memory->sem_SM);
//         sem_post(shared_memory->sem_fila);
//         pthread_mutex_unlock(&shared_memory->mutex_dispatcher);

//         sem_wait(shared_memory->sem_servers);

//         char temp[BUFSIZE];
//         int possivel = 0;

//         for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
//             if (possivel == 1)
//                 break;
//             sem_wait(shared_memory->sem_performace);
//             if (shared_memory->mode_cpu == 1) { // Modo Normal
//                 sem_post(shared_memory->sem_performace);

//                 sem_wait(shared_memory->sem_SM);
//                 if (servers[i].es_ativo == 0) {
//                     sem_post(shared_memory->sem_SM);
//                     sem_wait(shared_memory->sem_fila);
//                     printf("DEBUG: passou while, %d | %d | %d | %d | n_tarefas: %d\n", shared_memory->Num_es_ativos, servers[0].es_ativo, servers[1].es_ativo, servers[2].es_ativo, shared_memory->n_tarefas);
//                     sem_post(shared_memory->sem_fila);
//                     // TODO:

//                     if (received_msg.num_instrucoes / servers[i].mips1 < received_msg.max_tempo) { // FIXME: verficar esta condicao
//                         time_t tempo_final;
//                         time(&tempo_final);
//                         double tempo_espera = difftime(mktime(localtime(&tempo_final)), mktime(&received_msg.tempo_chegada));
//                         sem_wait(shared_memory->sem_fila);
//                         printf("DEBUG: passou while, %d | %d | %d | %d | n_tarefas: %d\n", shared_memory->Num_es_ativos, servers[0].es_ativo, servers[1].es_ativo, servers[2].es_ativo, shared_memory->n_tarefas);
//                         if (MQ.n_tarefas <= 1|| tempo_espera > 1000000)
//                             sem_post(shared_memory->sem_fila);
//                         else {
//                             sem_post(shared_memory->sem_fila);
//                             sem_wait(shared_memory->sem_SM);
//                             shared_memory->tempo_medio += tempo_espera;
//                             sem_post(shared_memory->sem_SM);
//                             printf("tempo espera!! -> %f\n", tempo_espera);
//                         }
//                         if (write(servers[i].fd[WRITE], &received_msg, sizeof(Task)) == -1) {
//                             perror("Erro ao escrever no pipe:");
//                         }
//                         sem_wait(shared_memory->sem_fila);
//                         shared_memory->n_tarefas--;
//                         pthread_cond_signal(&shared_memory->cond_monitor);
//                         sem_post(shared_memory->sem_fila);
//                         possivel = 1;
//                         break;
//                     }
//                 }

//             } else if (shared_memory->mode_cpu == 2) { // Modo HP //FIXME: BUGADO ATE AO PESCOÇO, MAS JA ESTEVE MAIS!!
//                 sem_post(shared_memory->sem_performace);
//                 sem_wait(shared_memory->sem_SM);
//                 sem_wait(shared_memory->sem_manutencao);
//                 if (servers[i].em_manutencao == 0 && servers[i].es_ativo < 2) {
//                     sem_post(shared_memory->sem_manutencao);
//                     sem_post(shared_memory->sem_SM);
//                     if ((received_msg.num_instrucoes / servers[i].mips1 < received_msg.max_tempo) || (received_msg.num_instrucoes / servers[i].mips2 < received_msg.max_tempo)) {
//                         time_t tempo_final;
//                         time(&tempo_final);
//                         double tempo_espera = difftime(mktime(localtime(&tempo_final)), mktime(&received_msg.tempo_chegada));
//                         sem_wait(shared_memory->sem_fila);
//                         printf("DEBUG: passou while, %d | %d | %d | %d | n_tarefas: %d\n", shared_memory->Num_es_ativos, servers[0].es_ativo, servers[1].es_ativo, servers[2].es_ativo, shared_memory->n_tarefas);
//                         if (MQ.n_tarefas <= 1 || tempo_espera > 1000000)
//                             sem_post(shared_memory->sem_fila);
//                         else {
//                             sem_post(shared_memory->sem_fila);
//                             sem_wait(shared_memory->sem_SM);
//                             shared_memory->tempo_medio += tempo_espera;
//                             sem_post(shared_memory->sem_SM);
//                             printf("tempo espera!! -> %f\n", tempo_espera);
//                         }
//                         // sem_wait(shared_memory->sem_estatisticas);
//                         if (write(servers[i].fd[WRITE], &received_msg, sizeof(Task)) == -1) {
//                             perror("Erro ao escrever no pipe:");
//                         }
//                         sem_wait(shared_memory->sem_fila);
//                         shared_memory->n_tarefas--;
//                         pthread_cond_signal(&shared_memory->cond_monitor);
//                         sem_post(shared_memory->sem_fila);
//                         possivel = 1;
//                         break;
//                     }
//                 } else {
//                     sem_post(shared_memory->sem_manutencao);
//                     continue;
//                 }
//             }
//         }
//         if (possivel == 0) {
//             char mensagem[200];
//             snprintf(mensagem, 200, "Tempo insuficiente para executar a tarefa %d", received_msg.idTarefa);
//             sem_wait(shared_memory->sem_fila);
//             shared_memory->n_tarefas--;
//             pthread_cond_signal(&shared_memory->cond_monitor);
//             sem_post(shared_memory->sem_fila);
//             sem_post(shared_memory->sem_servers);
//             log_msg(mensagem, 0);
//         }
//         sleep(0.4); // DEBUG: desbugar a vm necessita deste sleep ns pq
//     }

//     pthread_exit(NULL);
// }