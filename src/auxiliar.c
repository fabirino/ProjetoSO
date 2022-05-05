// Eduardo Figueiredo 2020213717
// Fábio Santos 2020212310

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

    // Inicializar o array que mostra o numero de ES ativos
    shared_memory->mode_cpu = 1;
    shared_memory->Num_es_ativos = 0;
    shared_memory->n_tarefas = 0;
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
                    strcpy(servers[i].nome, token);
                    servers[i].manutencoes = 0;
                    servers[i].tarefas_executadas = 0;
                    n++;
                }

                else if (n == 1) {
                    int ret;
                    ret = (int)strtol(token, &ptr, 10);
                    servers[i].mips1 = ret;
                    n++;
                } else if (n == 2) {
                    int ret;
                    ret = (int)strtol(token, &ptr, 10);
                    servers[i].mips2 = ret;
                    n++;
                }

                token = strtok(NULL, sep);
            }
            i++;
        }
        num++;
    }
    fclose(fich);
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        /* code */
        servers[i].es_ativo = 0;
    }
}

//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

void inicializar(base *pf) {
    pf->nos = (no_fila *)malloc(sizeof(no_fila) * shared_memory->QUEUE_POS);
    pf->n_tarefas = 0;
    // a fila está inicialmente vazia
    pf->entrada_lista = shared_memory->QUEUE_POS - 1;
    for (int i = 0; i < shared_memory->QUEUE_POS; i++)
        pf->nos[i].ocupado = false;
}

bool colocar(base *pf, Task tarefa, int prioridade) {
    int i, anterior, prox;
    // Procurar uma posição disponível
    for (i = shared_memory->QUEUE_POS - 1; i >= 0 && pf->nos[i].ocupado; i--)
        ;
    if (i < 0) {
        // fila cheia - não é possível inserir mais nada
        return false;
    }
    // colocar mensagem na fila
    pf->nos[i].tarefa = tarefa;
    pf->nos[i].prioridade = prioridade;

    // Procurar a posição onde a mensagem deve ficar
    if (!(pf->nos[pf->entrada_lista].ocupado)) {
        // fila vazia, inserir primeira mensagem
        pf->entrada_lista = i;
        pf->nos[i].mens_seguinte = -1;
    } else {
        // fila contém mensagens
        if (pf->nos[pf->entrada_lista].prioridade >= prioridade) {
            // inserir à entrada da lista
            pf->nos[i].mens_seguinte = pf->entrada_lista;
            pf->entrada_lista = i;
        } else {
            // procurar posição de inserção
            anterior = pf->entrada_lista;
            prox = pf->nos[pf->entrada_lista].mens_seguinte;
            while (prox >= 0 && pf->nos[prox].prioridade < prioridade) {
                anterior = prox;
                prox = pf->nos[prox].mens_seguinte;
            }
            if (prox < 0) {
                // inserir nos final da lista
                pf->nos[anterior].mens_seguinte = i;
                pf->nos[i].mens_seguinte = -1;
            } else {
                // inserir a meio da lista
                pf->nos[anterior].mens_seguinte = i;
                pf->nos[i].mens_seguinte = prox;
            }
        }
    }
    pf->nos[i].ocupado = true;
    return true;
}

bool retirar(base *pf, Task *ptarefa) {
    int i, j;
    if (!pf->nos[pf->entrada_lista].ocupado) {
        // lista vazia
        return false;
    }

    //  Procurar a última mensagem da lista
    j = -1;
    for (i = pf->entrada_lista; pf->nos[i].mens_seguinte != -1; i = pf->nos[i].mens_seguinte)
        j = i; // guardar a localização da mensagem anterior à que vai sair

    if (j != -1)
        // havia mais do que uma mensagem na lista
        pf->nos[j].mens_seguinte = -1;

    pf->nos[i].ocupado = false;
    *ptarefa = pf->nos[i].tarefa;
    pf->n_tarefas--;

    return true;
}

void reoorganizar(base *pf, time_t tempo) {
    // TODO: code here/*
}

//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

// Funcao que trata do CTRL-Z (imprime as estatisticas)
void SIGTSTP_HANDLER(int signum) {

    printf("\n------Estatisticas------\n");

    // Total de tarefas executadas
    int count = 0;
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        count += servers[i].tarefas_executadas;
    }
    printf("Total de tarefas executadas: %d\n", count);

    // TODO:Tempo medio de cada tarefa
    printf("Tempo medio de cada Tarefa: 0s\n");

    // Numero de tarefas executadas por cada E Server
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        printf("Tarefas executadas pelo Edge Server %d: %d\n", i, servers[i].tarefas_executadas);
    }

    // Numero de manutencoes de cada E Sever
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        printf("Numero de manutencoes do Edge Server %d: %d\n", i, servers[i].manutencoes);
    }

    // Num de tarefas nao executadas
    printf("Numero de tarefas nao executadas: %d\n", shared_memory->tarefas_descartadas);
}

// Funcao que trata do CTRL-C (termina o programa)
void SIGINT_HANDLER(int signum) {
    printf("\n");
    log_msg("Sinal SIGINT recebido", 0);
    // DEBUG: nao pode ser desta maneira, uma ideia é ter um array na shared memory em que 0 esta a funcionar e 1 esta ligado e verificar se estao a 0 e nao deixar comecar mais tarefas!!
    //  // Esperar que as threads dos Edge Servers terminem

    log_msg("Esperando as ultimas tarefas terminarem", 0); // TODO:
    // for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
    //     pthread_join(servers[i].vCPU[0], NULL);
    //     pthread_join(servers[i].vCPU[1], NULL);
    // }

    // TODO: terminar a MQ
    msgctl(MQid, IPC_RMID, 0);

    // Fechar os semaforos
    sem_close(shared_memory->sem_manutencao);
    sem_close(shared_memory->sem_tarefas);
    sem_close(shared_memory->sem_ficheiro);
    sem_close(shared_memory->sem_SM);
    sem_close(shared_memory->sem_servers);
    sem_unlink("SEM_MANUTENCAO");
    sem_unlink("SEM_TAREFAS");
    sem_unlink("SEM_FICHEIRO");
    sem_unlink("SEM_SM");
    sem_unlink("SEM_SERVERS");

    kill(shared_memory->maintenance_pid, SIGKILL);
    kill(shared_memory->monitor_pid, SIGKILL);
    kill(shared_memory->TM_pid, SIGKILL);

    // Close pipes
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        close(servers[i].fd[READ]);
        close(servers[i].fd[WRITE]);
    }

    // Remove shared_memory
    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, NULL);

    log_msg("O programa terminou\n", 0);
    // FIXME: sempre que ha um kill() o programa acaba imediatamente
    /* Guarantees that every process receives a SIGTERM , to kill them */
    kill(0, SIGTERM);
    exit(0);
}
