// Eduardo Figueiredo 2020213717
// Fábio Santos 2020212310

#include "auxiliar.h"

// criar a message queue interna a la maneta !!
base MQ;

void *p_dispatcher(void *lista) { // distribuição das tarefas

    // base *MQ = lista;
    // TODO: code here

    //#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=
    Task received_msg; // DEBUG:apenas testes!!!! IR PARA OS EDGE SERVERS !!!

    while (1) {
        // Verificar os ES que estao livres
        sem_wait(shared_memory->sem_SM);
        while (((shared_memory->mode_cpu == 1) && (shared_memory->Num_es_ativos >= shared_memory->EDGE_SERVER_NUMBER)) || (shared_memory->n_tarefas == 0) || ((shared_memory->mode_cpu == 2) && (shared_memory->Num_es_ativos == (shared_memory->EDGE_SERVER_NUMBER * 2)))) {
            sem_post(shared_memory->sem_SM);
            pthread_cond_wait(&shared_memory->cond_dispatcher, &shared_memory->mutex_dispatcher);
        }
        sem_wait(shared_memory->sem_servers);
        printf("passou while, %d | %d\n", shared_memory->Num_es_ativos, servers[0].es_ativo);

        for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
            sem_wait(shared_memory->sem_SM);
            if (shared_memory->mode_cpu == 1) {
                sem_post(shared_memory->sem_SM);
                if (servers[i].es_ativo == 0) {
                    // TODO:
                    if (retirar(&MQ, &received_msg)) {
                        if (write(servers[i].fd[WRITE], &received_msg, sizeof(Task)) == -1) {
                            perror("Erro ao escrever no pipe:");
                        }
                        sem_wait(shared_memory->sem_SM);
                        shared_memory->n_tarefas--;
                        sem_post(shared_memory->sem_SM);
                    } else {
                        printf("\nMQ vazia!!\n");
                    }
                    break;
                } else
                    continue;
            } else if (shared_memory->mode_cpu == 2) {
                sem_post(shared_memory->sem_SM);
                if (servers[i].es_ativo < 2) {
                    // TODO:
                    if (retirar(&MQ, &received_msg)) {
                        if (write(servers[i].fd[WRITE], &received_msg, sizeof(Task)) == -1) {
                            perror("Erro ao escrever no pipe:");
                        }
                        sem_wait(shared_memory->sem_SM);
                        shared_memory->n_tarefas--;
                        sem_post(shared_memory->sem_SM);
                    } else {
                        printf("\nMQ vazia!!\n");
                    }
                    break;
                } else
                    continue;
            }
        }
        sleep(1);
    }

    // QUESTION: este codigo ja nao vai funcionar pq trocarmos a mq pela maneta certo?
    // ANSWER: Aqui vamos ter que usar semafros e variaveis para quando um server estiver livre, este enviar uma tarefa para fazer por unamed pip,
    // ANTES de enviar verifica se ainda tem tempo para executar a tarefa, se nao tiver elimina-a da fila e escreve na log, caso tenha envia.

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
        strcpy(aux->nome_server, servers[i].nome);
        if (v == 0) {
            aux->mips_vcpu = servers[i].mips1;
        }
        if (v == 1) {
            aux->mips_vcpu = servers[i].mips2;
        }
        aux->n_vcpu = v + 1;
        snprintf(mensagem, 200, "CPU %d do Edge Server %s arrancou com capacidade de %d", aux->n_vcpu, aux->nome_server, aux->mips_vcpu);
        log_msg(mensagem, 0);
    }
    while (1) {
        /* code */
        // TODO: ver a mq da manutencao para ver se tem mensagens!!
        memset(mensagem, 0, 200);
        msgrcv(MQid, &mensagem, sizeof(mensagem), 1, IPC_NOWAIT);
        if (strlen(mensagem) != 0) {
            // TODO: codido da manutencao

            // sleep(tempoqualquer);
            continue;
        }
        // espera
        // receber a tarefa!
        Task tarefa; // FIXME: LER PIPE
        if (read(servers[i].fd[READ], &tarefa, sizeof(Task)) == -1) {
            perror("Erro ao ler do pipe");
            continue;
        } else {
            printf("[SERVER%d]  reveived, Task_id: %d number of requests: %d , max time: %d\n",
                   i, tarefa.idTarefa, tarefa.num_instrucoes, tarefa.max_tempo);
        }
        // sem_wait(shared_memory->sem_SM);
        // shared_memory->Num_es_ativos++;
        // servers[i].es_ativo++;
        // sem_post(shared_memory->sem_SM);
        // sem_post(shared_memory->sem_servers);
        // FIXME:

        aux->idTarefa = tarefa.idTarefa;
        aux->num_instrucoes = tarefa.num_instrucoes;
        if (shared_memory->mode_cpu == 1) { // FIXME: Ver o modo performace que so esta implementado para o modo 1!!
            memset(mensagem, 0, 200);
            strcpy(aux->nome_server, servers[i].nome);
            aux->mips_vcpu = servers[i].mips1;

            // DEBUG:  Normal: o Edge Server está em modo de economizar energia pelo que tem apenas 1 dos vCPUs ativo (o que tiver menos capacidade de processamento)
            if (servers[i].mips1 <= servers[i].mips2) {
                aux->n_vcpu = 1;
            } else {
                aux->n_vcpu = 2;
            }
            sem_wait(shared_memory->sem_SM);
            shared_memory->Num_es_ativos++;
            servers[i].es_ativo++;
            servers[i].cpu_ativo[aux->n_vcpu - 1] = 1;
            sem_post(shared_memory->sem_SM);
            sem_post(shared_memory->sem_servers);

            pthread_create(&servers[i].vCPU[aux->n_vcpu - 1], NULL, ES_routine, (void *)aux);

            pthread_join(servers[i].vCPU[aux->n_vcpu - 1], NULL);
        }

        else if (shared_memory->mode_cpu == 2) {
            memset(mensagem, 0, 200);
            strcpy(aux->nome_server, servers[i].nome);
            if (servers[i].cpu_ativo[0] == 0) {
                aux->mips_vcpu = servers[i].mips1;
                aux->n_vcpu = 1;

                sem_wait(shared_memory->sem_SM);
                shared_memory->Num_es_ativos++;
                servers[i].es_ativo++;
                servers[i].cpu_ativo[aux->n_vcpu - 1] = 1;
                sem_post(shared_memory->sem_SM);
                sem_post(shared_memory->sem_servers);

                pthread_create(&servers[i].vCPU[0], NULL, ES_routine, (void *)aux);

            } else if (servers[i].cpu_ativo[1] == 0) {
                aux->mips_vcpu = servers[i].mips2;
                aux->n_vcpu = 2;

                sem_wait(shared_memory->sem_SM);
                shared_memory->Num_es_ativos++;
                servers[i].es_ativo++;
                servers[i].cpu_ativo[aux->n_vcpu - 1] = 1;
                sem_post(shared_memory->sem_SM);
                sem_post(shared_memory->sem_servers);

                pthread_create(&servers[i].vCPU[1], NULL, ES_routine, (void *)aux);
            }

            // for (int j = 0; j < 2; j++) { // QUESTION: UII como fazer o pthrea_join se estiver 1 vcpu a espera
            //     pthread_join(servers[i].vCPU[j], NULL);
            // }
        }
    }
}

void *p_scheduler(void *lista) { // gestão do escalonamento das tarefas
    // TODO: code here
    // base MQ = *((base *)lista);

    // Abrir pipe para ler
    int fd;

    //-------------------------
    while (1) {
        if ((fd = open(PIPE_NAME, O_RDWR)) < 0) {
            perror("Nao pode abrir o pipe para ler:");
            exit(0);
        }
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
                    tarefa.num_instrucoes = atoi(token);
                else {
                    tarefa.max_tempo = atoi(token);
                    printf("[SERVER] Read %d bytes: reveived, Task_id: %d number of requests: %d , max time: %d\n",
                           r, tarefa.idTarefa, tarefa.num_instrucoes, tarefa.max_tempo);
                    // TODO:  colocar a tarefa, tem que reorganizar a fila, ou seja, diminuir a prioridade e verificar se ainda
                    // tem tempo para executar as tarefas, diminuir a prioridade o tempo que ja passou desde a ultima vez que colocou!!
                    if (colocar(&MQ, tarefa, tarefa.max_tempo) == false) {
                        char temp[BUFSIZE];
                        snprintf(temp, BUFSIZE, "SCHEDULER: Prazo maximo de execucao da tarefa %d atingido", tarefa.idTarefa);
                        log_msg(temp, 0);
                        shared_memory->tarefas_descartadas++;
                    } else {
                        pthread_mutex_lock(&shared_memory->mutex_dispatcher);
                        MQ.n_tarefas++;
                        sem_wait(shared_memory->sem_SM);
                        shared_memory->n_tarefas++;
                        sem_post(shared_memory->sem_SM);
                        pthread_cond_signal(&shared_memory->cond_dispatcher);
                        pthread_mutex_unlock(&shared_memory->mutex_dispatcher);
                        char temp[BUFSIZE];
                        snprintf(temp, BUFSIZE, "SCHEDULER: Tarefa inserida na fila nº %d", MQ.n_tarefas);
                        log_msg(temp, 0);
                    }
                }
                kappa++;
            }
            token = strtok(NULL, ";");
        }

        close(fd);
    }
    pthread_exit(NULL);
}

void task_manager() {

    // inicicalizar unnamed pipes
    // FIXME: BUGADO!!!
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        if (pipe(servers[i].fd) != 0) {
            erro("Erro ao criar os unnamed pipes!");
        }
    }

    inicializar(&MQ);

    // Criar um processo para cada Edge Server
    char teste[100];
    memset(teste, 0, 100);

    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        if ((servers[i].pid = fork()) == 0) {
            snprintf(teste, 100, "Edge server %d arrancou", i + 1);
            log_msg(teste, 0);
            server(i);
            exit(0);
        }
    }

    // Criação da thread scheduler
    pthread_t scheduler;
    pthread_create(&scheduler, NULL, p_scheduler, NULL); // Criação da thread scheduler
    log_msg("Criação da thread scheduler", 0);

    // TODO: "A thread dispatcher é ativada sempre que 1 vCPU fica livre e desde que existam tarefas por realizar"
    // Criação da thread dispatcher
    pthread_t dispatcher;
    pthread_create(&dispatcher, NULL, p_dispatcher, NULL); // Criação da thread dispatcher
    log_msg("Criação da thread dispatcher", 0);

    pthread_join(scheduler, NULL);
    pthread_join(dispatcher, NULL);
}

// Funcao encarregue de executar as tarefas do Edge Server
void *ES_routine(void *t) {
    argumentos aux = *(argumentos *)t;
    // printf("CPU %d do Edge Server %s arrancou com capacidade de %d\n", aux.n_vcpu, aux.nome_server, aux.mips_vcpu);

    // Executar a tarefa
    int tempo_execucao = aux.num_instrucoes / aux.mips_vcpu;
    sleep(tempo_execucao);

    char mensagem[BUFSIZE];
    snprintf(mensagem, BUFSIZE, "SERVER_%d: Task %d completada", aux.ES_num, aux.idTarefa);
    log_msg(mensagem, 0);

    sem_wait(shared_memory->sem_SM);
    servers[aux.ES_num].tarefas_executadas++;
    servers[aux.ES_num].es_ativo--;
    servers[aux.ES_num].cpu_ativo[aux.n_vcpu - 1] = 0;
    shared_memory->Num_es_ativos--;
    sem_post(shared_memory->sem_SM);
    pthread_mutex_lock(&shared_memory->mutex_dispatcher);
    pthread_cond_signal(&shared_memory->cond_dispatcher);
    pthread_mutex_unlock(&shared_memory->mutex_dispatcher);

    pthread_exit(NULL);
}