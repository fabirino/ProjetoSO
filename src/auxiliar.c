// Eduardo Figueiredo 2020213717
// Fábio Santos 2020212310

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "auxiliar.h"

// devolve o tempo formatado
void time_now(char *string) {
    time_t tempo = time(NULL);
    if (tempo == -1) {
        erro("Erro na funcao time():");
    }

    struct tm *ptm = localtime(&tempo);

    if (ptm == NULL) {
        erro("Erro na funcao localtime():");
    }

    snprintf(string, 10, "%02d:%02d:%02d ", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

// a funcao escreve no ficheiro log da primeira vez e nas proximas da append
void log_msg(char *msg, int first_time) {
    sem_wait(shared_memory->sem_ficheiro);

    char mensagem[BUFSIZE];
    time_now(mensagem);

    strcat(mensagem, msg);

    printf("%s\n", mensagem);

    if (first_time == 1) {
        FILE *log = fopen("log.txt", "w");
        if (log == NULL) {
            erro("Erro ao abrir o ficheiro");
        }
        fprintf(log, "%s", mensagem);
        fprintf(log, "\n");
        fclose(log);
    } else if (first_time == 0) {
        FILE *log = fopen("log.txt", "a");
        if (log == NULL) {
            erro("Erro ao abrir o ficheiro");
        }
        fprintf(log, "%s", mensagem);
        fprintf(log, "\n");
        fclose(log);
    }
    sem_post(shared_memory->sem_ficheiro);
}

void erro(char *msg) {
    perror(msg);
    log_msg("O programa terminou devido a um erro.", 0);
    exit(1);
}

// funcao que le o ficheiro de configuracao
void config(char *path) {
    FILE *fich = fopen(path, "r");
    assert(fich);

    char line[20];

    fscanf(fich, "%s", line);
    shared_memory->QUEUE_POS = atoi(line);

    fscanf(fich, "%s", line);
    shared_memory->MAX_WAIT = atoi(line);

    fscanf(fich, "%s", line);
    shared_memory->EDGE_SERVER_NUMBER = atoi(line);
    if (shared_memory->EDGE_SERVER_NUMBER < 2) {
        log_msg("Numero insuficiente de Edge Servers(>=2)!!", 1);
        exit(0);
    }
    fclose(fich);
}

// funcao que cria os edge servers com base nos dados obtidos no ficheiro config
void createEdgeServers(char *path) {

    FILE *fich = fopen(path, "r");
    assert(fich);

    char line[20];
    const char sep[2] = ",";

    int num = 0;
    int i = 0;
    char *ptr;
    while (fscanf(fich, "%[^\n] ", line) != EOF) {
        if (num > 2) {
            char *token;
            token = strtok(line, sep);
            int n = 0;
            while (token != NULL) {
                if (n == 0) {
                    strcpy(shared_memory->servers[i].nome, token);
                    n++;
                }

                else if (n == 1) {
                    int ret;
                    ret = (int)strtol(token, &ptr, 10);
                    shared_memory->servers[i].mips1 = ret;
                    n++;
                } else if (n == 2) {
                    int ret;
                    ret = (int)strtol(token, &ptr, 10);
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

void *p_scheduler() { // gestão do escalonamento das tarefas
    // TODO:code here
    pthread_exit(NULL);
}

void *p_dispatcher() { // distribuição das tarefas
    // TODO:code here
    pthread_exit(NULL);
}

void task_menager() {

    // Criar um processo para cada Edge Server
    char teste[100];
    memset(teste, 0, 100);
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        if ((shared_memory->servers[i].pid = fork()) == 0) {
            snprintf(teste, 100, "Edge server %d arrancou", i + 1);
            log_msg(teste, 0);
            // DEBUG:
            Server(i);
            exit(0);
        }
    }

    // Abrir pipe para ler
    int fd;
    if ((fd = open(PIPE_NAME, O_RDWR)) < 0) {
        perror("Nao pode abrir o pipe para ler:");
        exit(0);
    }

    //-------------------------
    //DEBUG: acho que isto faz-se na thread schedular! DUVIDAS NESTA PARTE POR ISSO FICA POR AQUI xD
    int r;
    r = read(fd, &n, sizeof(numbers));
    if (r < 0)
        perror("Named Pipe");
    printf("[SERVER] Read %d bytes: reveived (%d,%d), adding it: %d\n",
           r, n.a, n.b, n.a + n.b);
    //----------------------------

    pthread_t scheduler;
    pthread_create(&scheduler, NULL, p_scheduler, NULL); // Criação da thread scheduler
    memset(teste, 0, 100);
    snprintf(teste, 100, "Criação da thread scheduler");
    log_msg(teste, 0);

    pthread_t dispatcher;
    pthread_create(&dispatcher, NULL, p_dispatcher, NULL); // Criação da thread dispatcher
    memset(teste, 0, 100);
    snprintf(teste, 100, "Criação da thread dispatcher");
    log_msg(teste, 0);

    pthread_join(scheduler, NULL);
    pthread_join(dispatcher, NULL);
}

// Funcao encarregue de executar as tarefas de cada E-Server
void Server(int i) {
    char mensagem[200];
    argumentos aux;
    for (int v = 0; v < 2; v++) {
        argumentos *aux = (argumentos *)malloc(sizeof(argumentos));
        strcpy(aux->nome_server, shared_memory->servers[i].nome);
        if (v == 0) {
            aux->capacidade_vcpu = shared_memory->servers[i].mips1;
        }
        if (v == 1) {
            aux->capacidade_vcpu = shared_memory->servers[i].mips2;
        }
        aux->n_vcpu = v + 1;
        snprintf(mensagem, 200, "CPU %d do Edge Server %s arrancou com capacidade de %d", aux->n_vcpu, aux->nome_server, aux->capacidade_vcpu);
        log_msg(mensagem, 0);
        pthread_create(&shared_memory->servers[i].vCPU[v], NULL, function, (void *)aux);
        memset(mensagem, 0, 200);
    }
    for (int j = 0; j < 2; j++) {
        pthread_join(shared_memory->servers[i].vCPU[j], NULL);
    }
}

// Funcao encarregue de executar as tarefas do Edge Server
void *function(void *t) {
    argumentos aux = *(argumentos *)t;
    // printf("CPU %d do Edge Server %s arrancou com capacidade de %d\n", aux.n_vcpu, aux.nome_server, aux.capacidade_vcpu);

    pthread_exit(NULL);
}

// Funcao que trata do CTRL-Z (imprime as estatisticas)
void SIGTSTP_HANDLER(int signum) {

    printf("------Estatisticas------\n");

    // TODO: descobrir que estatisticas imprimir
}

// Funcao que trata do CTRL-C (termina o programa)
void SIGINT_HANDLER(int signum) { // TODO: terminar a MQ

    // Esperar que as threads dos Edge Servers terminem
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        pthread_join(shared_memory->servers[i].vCPU[0], NULL);
        pthread_join(shared_memory->servers[i].vCPU[1], NULL);
    }

    // MQ
    // msgctl(shared_memory->mqid)

    // Fechar os semaforos
    sem_close(shared_memory->sem_manutencao);
    sem_close(shared_memory->sem_tarefas);
    sem_close(shared_memory->sem_ficheiro);
    sem_unlink("SEM_MANUTENCAO");
    sem_unlink("SEM_TAREFAS");
    sem_unlink("SEM_FICHEIRO");
    exit(0);

    log_msg("O programa terminou\n", 0);
}
