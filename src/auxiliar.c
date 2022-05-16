// Eduardo Figueiredo 2020213717
// Fábio Santos 2020212310

#include "auxiliar.h"

/* Funcoes da lista ligada*/

void inicializar(base *pf) {
    pf->first_node = NULL;
    pf->n_tarefas = 0;
}

void reoorganizar(base *pf) {

    struct timeval stop_time;
    gettimeofday(&stop_time, NULL);

    no_fila *aux = pf->first_node;
    no_fila *anterior = NULL;
    char mensagem[BUFSIZE];

    while (aux != NULL) {
        float intervalo = (float)(stop_time.tv_sec - aux->tarefa.tempo_chegada.tv_sec);
        intervalo += (stop_time.tv_usec - aux->tarefa.tempo_chegada.tv_usec) * 1e-6;
        float max = aux->tarefa.max_tempo - intervalo;

        if (intervalo < 1000000) {
            if (max <= 0) { // Retirar as mensagens cujo tempo de execucao ja expirou

                if (anterior == NULL) { // Primeiro no
                    memset(mensagem, 0, BUFSIZE);
                    snprintf(mensagem, BUFSIZE, "[SCHEDULAR] a tarefa com id %d foi descartada pois ja nao tem tempo para ser executada ||%f!", aux->tarefa.idTarefa, max);
                    log_msg(mensagem, 0);
                    pf->n_tarefas--;
                    shared_memory->n_tarefas--;
                    shared_memory->tarefas_descartadas++;

                    pf->first_node = aux->next;
                    free(aux);

                } else { // Outros nos
                    memset(mensagem, 0, BUFSIZE);
                    snprintf(mensagem, BUFSIZE, "[SCHEDULAR] a tarefa com id %d foi descartada pois ja nao tem tempo para ser executada ||%f!", aux->tarefa.idTarefa, max);
                    log_msg(mensagem, 0);
                    pf->n_tarefas--;
                    shared_memory->n_tarefas--;
                    shared_memory->tarefas_descartadas++;

                    anterior->next = aux->next;
                    free(aux);
                }

            } else { // Diminuir os tempos de execucao
                aux->prioridade = (int)aux->tarefa.max_tempo - (int)intervalo;
            }
        }
        anterior = aux;
        aux = aux->next;
    }
}

bool colocar(base *pf, Task tarefa) {

    if (pf->n_tarefas >= 50) {
        return false;
    }

    struct no_fila *aux, *prox, *anterior;

    // Obter espaço para um novo nó
    aux = (struct no_fila *)malloc(sizeof(struct no_fila));
    if (aux == NULL)
        // não há espaço
        return false;

    aux->tarefa = tarefa;
    aux->prioridade = (int)tarefa.max_tempo;
    int prioridade = (int)tarefa.max_tempo;
    aux->next = NULL;

    // Procurar a posição onde a mensagem deve ficar
    if (pf->first_node == NULL) {
        // fila vazia, é a primeira mensagem
        pf->first_node = aux;
    } else {
        // fila contém mensagens
        if (pf->first_node->prioridade <= prioridade) {
            // inserir à entrada da lista
            aux->next = pf->first_node;
            pf->first_node = aux;
        } else {
            // procurar posição de inserção
            anterior = pf->first_node;
            prox = pf->first_node->next;
            while (prox != NULL && prox->prioridade > prioridade) {
                anterior = prox;
                prox = prox->next;
            }
            if (prox == NULL) {
                // inserir à saída da lista
                anterior->next = aux;
            } else {
                // inserir a meio da lista
                anterior->next = aux;
                aux->next = prox;
            }
        }
    }
    return true;
}

bool retirar(base *pf, Task *ptarefa) {

    if (pf->first_node == NULL) {
        return false;
    }

    no_fila *aux = pf->first_node;

    aux = pf->first_node;

    *ptarefa = aux->tarefa;
    pf->n_tarefas--;
    pf->first_node = aux->next;
    free(aux);

    return true;
}

//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

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

    int q_pos = 0;
    int max = 0;
    int n_servers = 0;

    char line[20];

    fscanf(fich, "%s", line);
    q_pos = atoi(line);

    fscanf(fich, "%s", line);
    max = atoi(line);

    fscanf(fich, "%s", line);
    n_servers = atoi(line);
    if (n_servers < 2) {
        log_msg("Numero insuficiente de Edge Servers(>=2)!!", 1);
        exit(0);
    }
    fclose(fich);

    // Create shared memory
    shmid = shmget(IPC_PRIVATE, sizeof(SM) + sizeof(Edge_Server) * n_servers, IPC_CREAT | 0766);
    // Attach shared memory
    shared_memory = (SM *)shmat(shmid, NULL, 0);
    servers = (Edge_Server *)(shared_memory + 1);

    shared_memory->QUEUE_POS = q_pos;
    shared_memory->MAX_WAIT = max;
    shared_memory->EDGE_SERVER_NUMBER = n_servers;

    // Inicializar variaveis da SM
    shared_memory->mode_cpu = 1;
    shared_memory->Num_es_ativos = 0;
    shared_memory->n_tarefas = 0;
    shared_memory->tempo_medio = 0;
    shared_memory->em_manutencao = 0;
    shared_memory->exit = 0;
}

// funcao que cria os edge servers com base first_node dados obtidos no ficheiro config
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
        servers[i].es_ativo = 0;
        for (int l = 0; l < 2; l++) {
            servers[i].cpu_ativo[l] = 0;
        }
        servers[i].em_manutencao = 0;
        servers[i].manutencao = 0;

        // considerar o vcpu1 o menos eficaz
        if (servers[i].mips2 > servers[i].mips1) {
            int aux = servers[i].mips1;
            servers[i].mips1 = servers[i].mips2;
            servers[i].mips2 = aux;
        }
    }
}

void *out() {

    pid_t pidd = getpid();

    pthread_mutex_lock(&shared_memory->mutex_exit);
    pthread_cond_wait(&shared_memory->cond_exit, &shared_memory->mutex_exit);
    pthread_mutex_unlock(&shared_memory->mutex_exit);

    if (pidd == shared_memory->TM_pid) {
        printf("TASK Manager a Terminar!\n");
        pthread_cancel(shared_memory->scheduler);
        printf("Schedular a Terminar\n");
        pthread_cancel(shared_memory->dispatcher);
        printf("Dispatcher a Terminar\n");

    } else if (pidd == shared_memory->monitor_pid) {
        printf("Monitor a Terminar!\n");
    } else if (pidd == shared_memory->maintenance_pid) {
        printf("Maintenance a Terminar!\n");
    } else {
        for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
            if (pidd == servers[i].pid) {
                printf("Server%d a Terminar!\n",i);
                pthread_cancel(servers[i].vCPU[0]);
                pthread_cancel(servers[i].vCPU[1]);
            }
        }
    }
    exit(0);
}

// Funcao que trata do CTRL-Z (imprime as estatisticas)
void SIGTSTP_HANDLER(int signum) {

    printf("\n");
    log_msg("Sinal SIGTSTP recebido", 0);
    printf("------Estatisticas------\n");

    // Total de tarefas executadas
    int count = 0;
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        count += servers[i].tarefas_executadas;
    }
    printf("Total de tarefas executadas: %d\n", count);

    // Tempo medio de cada tarefa

    float tempo_medio = (float)shared_memory->tempo_medio / (float)count;

    printf("Tempo medio de espera das Tarefas: %4fs\n", tempo_medio);

    // Numero de tarefas executadas por cada E Server
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        printf("Tarefas executadas pelo Edge Server %d: %d\n", i + 1, servers[i].tarefas_executadas);
    }

    // Numero de manutencoes de cada E Sever
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        printf("Numero de manutencoes do Edge Server %d: %d\n", i + 1, servers[i].manutencoes);
    }

    // Num de tarefas nao executadas
    printf("Numero de tarefas nao executadas: %d\n", shared_memory->tarefas_descartadas);

    int value;
    sem_getvalue(shared_memory->sem_fila, &value);
    printf("fila-> %d, ", value);
    sem_getvalue(shared_memory->sem_manutencao, &value);
    printf("manutencao -> %d , ", value);
    sem_getvalue(shared_memory->sem_SM, &value);
    printf("sm-> %d, ", value);
    sem_getvalue(shared_memory->sem_servers, &value);
    printf("servers->%d, ", value);
    // sem_getvalue(shared_memory->sem_tarefas, &value);
    // printf("tarefas ->%d, ", value);
    sem_getvalue(shared_memory->sem_performace, &value);
    printf("performace -> %d\n", value);
}

// Funcao que trata do CTRL-C (termina o programa)
void SIGINT_HANDLER(int signum) {
    printf("\n");
    log_msg("Sinal SIGINT recebido", 0);
    // Esperar que as threads dos Edge Servers terminem
    sem_wait(shared_memory->sem_SM);
    shared_memory->exit = 1;
    sem_post(shared_memory->sem_SM);

    sem_wait(shared_memory->sem_SM);
    pthread_mutex_lock(&shared_memory->mutex_dispatcher);
    while (shared_memory->Num_es_ativos > 0) {

        sem_post(shared_memory->sem_SM);

        pthread_cond_wait(&shared_memory->cond_dispatcher, &shared_memory->mutex_dispatcher);
        sem_wait(shared_memory->sem_SM);
    }
    sem_post(shared_memory->sem_SM);

    pthread_mutex_unlock(&shared_memory->mutex_dispatcher);
    

    log_msg("Esperando as ultimas tarefas terminarem", 0); // TODO:

    pthread_cond_broadcast(&shared_memory->cond_exit);

    while (wait(NULL) > 0)
        ;

    // TODO: terminar a MQ
    msgctl(MQid, IPC_RMID, 0);

    // Fechar os semaforos
    sem_close(shared_memory->sem_manutencao);
    // sem_close(shared_memory->sem_tarefas);
    sem_close(shared_memory->sem_ficheiro);
    sem_close(shared_memory->sem_SM);
    sem_close(shared_memory->sem_servers);
    sem_close(shared_memory->sem_performace);
    sem_close(shared_memory->sem_fila);
    sem_unlink("SEM_MANUTENCAO");
    // sem_unlink("SEM_TAREFAS");
    sem_unlink("SEM_FICHEIRO");
    sem_unlink("SEM_SM");
    sem_unlink("SEM_SERVERS");
    sem_unlink("SEM_PERFORMACE");
    sem_unlink("SEM_FILA");


    // Close pipes
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        close(servers[i].fd[READ]);
        close(servers[i].fd[WRITE]);
    }


    // Remove shared_memory
    if (shmdt(shared_memory)== -1){
          perror("acoplamento impossivel") ;
     }
    if ( shmctl(shmid, IPC_RMID,0) == -1){
          perror("destruicao impossivel") ;
     }
    // log_msg("O programa terminou\n", 0);
    // FIXME: sempre que ha um kill() o programa acaba imediatamente
    /* Guarantees that every process receives a SIGTERM , to kill them */
    exit(0);
}
