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
    shared_memory->ES_ativos = (int *)malloc(sizeof(int) * shared_memory->EDGE_SERVER_NUMBER);
    shared_memory->CPU_ativos = 1;
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
                    shared_memory->servers[i].manutencoes = 0;
                    shared_memory->servers[i].tarefas_executadas = 0;
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

//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

void inicializar(base *pf, int tamanho) {
    pf->tam = tamanho;
    // a fila está inicialmente vazia
    pf->entrada_lista = tamanho - 1;
    for (int i = 0; i < tamanho; i++)
        pf->nos[i].ocupado = false;
}

bool colocar(base *pf, Task tarefa, int prioridade) {
    int i, anterior, prox;
    // Procurar uma posição disponível
    for (i = pf->tam - 1; i >= 0 && pf->nos[i].ocupado; i--)
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

    return true;
}

void reoorganizar(base *pf, time_t tempo) {
    // TODO: code here/*
}

//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

void *p_scheduler(void *lista) { // gestão do escalonamento das tarefas
    // TODO: code here
    base *MQ = lista;

    // Abrir pipe para ler
    int fd;
    if ((fd = open(PIPE_NAME, O_RDWR)) < 0) {
        perror("Nao pode abrir o pipe para ler:");
        exit(0);
    }

    //-------------------------
    while (1) {
        Task tarefa;
        int r;
        char mensagem[BUFSIZE];
        r = read(fd, &mensagem, sizeof(mensagem));
        if (r < 0)
            perror("Named Pipe");
        int kappa = 0;
        char *token;
        token = strtok(mensagem, ";");

        while (token != NULL) {
            if (!strcmp(token, "EXIT") && kappa == 0) {
                // Acaba o programa
                SIGINT_HANDLER;
                break;
            } else if (!strcmp(token, "STATS") && kappa == 0) {
                // Imprime as estatisticas
                SIGTSTP_HANDLER;
                break;
            } else {
                if (kappa == 0)
                    tarefa.idTarefa = atoi(token);

                else if (kappa == 1)
                    tarefa.num_pedidos = atoi(token);
                else
                    tarefa.max_tempo = atoi(token);
                kappa++;
            }
            token = strtok(NULL, ";");
        }
        if (kappa != 0) {
            printf("[SERVER] Read %d bytes: reveived, Task_id: %d number of requests: %d , max time: %d\n",
                   r, tarefa.idTarefa, tarefa.num_pedidos, tarefa.max_tempo);

            colocar(MQ, tarefa, tarefa.max_tempo); // TODO:  colocar a tarefa, tem que reorganizar a fila, ou seja, diminuir a prioridade e verificar se ainda
            MQ->tam++;                             // tem tempo para executar as tarefas,diminuir a prioridade o tempo que ja passou desde a ultima vez que colocou!!
        }

        if (0) { // TODO: verificar se a condicao consegue ser executada no MAX_WAIT
            char temp[BUFSIZE];
            snprintf(temp, BUFSIZE, "SCHEDULER: Prazo maximo de execucao da tarefa %d atingido", tarefa.idTarefa);
            log_msg(temp, 0);
            shared_memory->tarefas_descartadas++;
        }
    }
    pthread_exit(NULL);
}

void *p_dispatcher(void *lista) { // distribuição das tarefas

    base *MQ = lista;
    // TODO: code here

    //#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=
    Task received_msg; // DEBUG:apenas testes!!!! IR PARA OS EDGE SERVERS !!!

    // while (1) {
    //     // Verificar os ES que estao livres
    //     while (shared_memory->Num_es_ativos == 3) {
    //         pthread_cond_wait(&shared_memory->pthread_cond, &shared_memory->pthread_sem);
    //     }

    //     for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
    //         if (shared_memory->ES_ativos[i] == 0) {
    //             // TODO:
    //             if (pipe(shared_memory->pipes_fd[i]) == -1) {
    //                 perror("Erro ao abrir o pipe");
    //             }
    //             retirar(MQ,&received_msg);//TODO: verificar se ainda tem tempo para executar a mesma!!!
    //             if (write(shared_memory->pipes_fd[i][0], &received_msg, sizeof(Task)) == -1) {
    //                 perror("Erro ao ler do pipe");
    //             }
    //             break;
    //         }
    //     }
    // }

    // QUESTION: este codigo ja nao vai funcionar pq trocarmos a mq pela maneta certo?
    // ANSWER: Aqui vamos ter que usar semafros e variaveis para quando um server estiver livre, este enviar uma tarefa para fazer por unamed pip,
    // ANTES de enviar verifica se ainda tem tempo para executar a tarefa, se nao tiver elimina-a da fila e escreve na log, caso tenha envia.

    // while (1) {
    //     /* TO COMPLETE: Receive messages with the higher priority available */
    //     msgrcv(MQid, &received_msg, sizeof(priority_msg),-10000 , 0);
    //     /* TO COMPLETE: end */

    //     printf("[high_priority_first] Received message [%d]!\n", received_msg.msg_number);
    //     sleep(5);
    // }
    //#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=

    pthread_exit(NULL);
}

// Funcao encarregue de executar as tarefas de cada E-Server
void server(int i) { // TODO: recebe por um unamed pipe a tarefa a executar e se tiver em modo performace divide a tarefa pelas 2 threads,se tiver em modo 1 executa-a com 1 thread!
    char mensagem[200];
    argumentos *aux = (argumentos *)malloc(sizeof(argumentos));
    aux->ES_num = i;
    for (int v = 0; v < 2; v++) { // DEBUG: ver se colocamos isto no monitor !!
        memset(mensagem, 0, 200);
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
    }
    while (1) {
        /* code */
        // TODO: ver a mq da manutencao para ver se tem mensagens!!
        // espera

        // receber a tarefa!

        Task tarefa; // FIXME: LER PIPE
        // if (read(shared_memory->pipes_fd[i][0], &tarefa, sizeof(Task)) == -1) {
        //     perror("Erro ao ler do pipe");
        //     continue;
        // }
        // FIXME:
        aux->idTarefa = 1;
        if (shared_memory->CPU_ativos == 1) { // TODO: mandar por parametros para a thread a task !!
            memset(mensagem, 0, 200);
            strcpy(aux->nome_server, shared_memory->servers[i].nome);
            aux->capacidade_vcpu = shared_memory->servers[i].mips1;

            aux->n_vcpu = 1;
            pthread_create(&shared_memory->servers[i].vCPU[0], NULL, ES_routine, (void *)aux);

            pthread_join(shared_memory->servers[i].vCPU[0], NULL);
        }

        if (shared_memory->CPU_ativos == 2) {
            for (int v = 0; v < 2; v++) { // mode performace
                memset(mensagem, 0, 200);
                strcpy(aux->nome_server, shared_memory->servers[i].nome);
                if (v == 0) {
                    aux->capacidade_vcpu = shared_memory->servers[i].mips1;
                }
                if (v == 1) {
                    aux->capacidade_vcpu = shared_memory->servers[i].mips2;
                }
                aux->n_vcpu = v + 1;
                pthread_create(&shared_memory->servers[i].vCPU[v], NULL, ES_routine, (void *)aux);
            }

            for (int j = 0; j < 2; j++) {
                pthread_join(shared_memory->servers[i].vCPU[j], NULL);
            }
        }
    }
}

void task_manager(base *MQ) {

    // Criar um processo para cada Edge Server
    char teste[100];
    memset(teste, 0, 100);

    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        if ((shared_memory->servers[i].pid = fork()) == 0) {
            snprintf(teste, 100, "Edge server %d arrancou", i + 1);
            log_msg(teste, 0);
            server(i);
            exit(0);
        }
    }

    // Criação da thread scheduler
    pthread_t scheduler;
    pthread_create(&scheduler, NULL, p_scheduler, (void *)&MQ); // Criação da thread scheduler
    log_msg("Criação da thread scheduler", 0);

    // Criação da thread dispatcher
    pthread_t dispatcher;
    pthread_create(&dispatcher, NULL, p_dispatcher, (void *)&MQ); // Criação da thread dispatcher
    log_msg("Criação da thread dispatcher", 0);

    pthread_join(scheduler, NULL);
    pthread_join(dispatcher, NULL);
}

// Funcao encarregue de executar as tarefas do Edge Server
void *ES_routine(void *t) {
    argumentos aux = *(argumentos *)t;
    // printf("CPU %d do Edge Server %s arrancou com capacidade de %d\n", aux.n_vcpu, aux.nome_server, aux.capacidade_vcpu);

    // TODO: code here

    char mensagem[BUFSIZE];
    snprintf(mensagem, BUFSIZE, "SERVER_%d: Task %d completada", aux.ES_num, aux.idTarefa);
    log_msg(mensagem, 0);
    shared_memory->servers[aux.ES_num].tarefas_executadas++;
    pthread_exit(NULL);
}

// Funcao que trata do CTRL-Z (imprime as estatisticas)
void SIGTSTP_HANDLER(int signum) {

    printf("\n------Estatisticas------\n");

    // Total de tarefas executadas
    int count = 0;
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        count += shared_memory->servers[i].tarefas_executadas;
    }
    printf("Total de tarefas executadas: %d\n", count);

    // TODO:Tempo medio de cada tarefa
    printf("Tempo medio de cada Tarefa: 0s\n");

    // Numero de tarefas executadas por cada E Server
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        printf("Tarefas executadas pelo Edge Server %d: %d\n", i, shared_memory->servers[i].tarefas_executadas);
    }

    // Numero de manutencoes de cada E Sever
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        printf("Numero de manutencoes do Edge Server %d: %d\n", i, shared_memory->servers[i].manutencoes);
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
    //     pthread_join(shared_memory->servers[i].vCPU[0], NULL);
    //     pthread_join(shared_memory->servers[i].vCPU[1], NULL);
    // }

    // TODO: terminar a MQ
    msgctl(MQid, IPC_RMID, 0);

    // FIXME: sempre que ha um kill() o programa acaba imediatamente
    /* Guarantees that every process receives a SIGTERM , to kill them */
    kill(0, SIGTERM);

    // Fechar os semaforos
    sem_close(shared_memory->sem_manutencao);
    sem_close(shared_memory->sem_tarefas);
    sem_close(shared_memory->sem_ficheiro);
    sem_unlink("SEM_MANUTENCAO");
    sem_unlink("SEM_TAREFAS");
    sem_unlink("SEM_FICHEIRO");
    sem_close(shared_memory->sem_SM);
    sem_unlink("SEM_SM");

    kill(shared_memory->maintenance_pid, SIGKILL);
    kill(shared_memory->monitor_pid, SIGKILL);
    kill(shared_memory->TM_pid, SIGKILL);

    // Close pipes
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        close(shared_memory->servers[i].fd[0]);
        close(shared_memory->servers[i].fd[1]);
    }

    // Remove shared_memory
    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, NULL);

    log_msg("O programa terminou\n", 0);
    exit(0);
}
