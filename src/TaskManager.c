// Eduardo Figueiredo 2020213717
// Fábio Santos 2020212310

#include "auxiliar.h"

// criar a message queue interna a la maneta !!
base MQ;

void *p_dispatcher(void *lista) { // distribuição das tarefas

    Task received_msg; // DEBUG:apenas testes!!!! IR PARA OS EDGE SERVERS !!!

    while (1) {
        // Verificar os ES que estao livres
        sem_wait(shared_memory->sem_SM);
        sem_wait(shared_memory->sem_performace);
        sem_wait(shared_memory->sem_fila);
        pthread_mutex_lock(&shared_memory->mutex_dispatcher);
        while (((shared_memory->mode_cpu == 1) && (shared_memory->Num_es_ativos >= shared_memory->EDGE_SERVER_NUMBER)) || (shared_memory->n_tarefas == 0) || ((shared_memory->mode_cpu == 2) && (shared_memory->Num_es_ativos == (shared_memory->EDGE_SERVER_NUMBER * 2)))) {
            printf("DEBUG: while, %d | %d | %d | %d | n_tarefas: %d\n", shared_memory->Num_es_ativos, servers[0].es_ativo, servers[1].es_ativo, servers[2].es_ativo, shared_memory->n_tarefas);
            sem_post(shared_memory->sem_performace);
            sem_post(shared_memory->sem_SM);
            sem_post(shared_memory->sem_fila);
            pthread_cond_wait(&shared_memory->cond_dispatcher, &shared_memory->mutex_dispatcher);
        }
        sem_post(shared_memory->sem_performace);
        sem_post(shared_memory->sem_SM);
        sem_post(shared_memory->sem_fila);
        pthread_mutex_unlock(&shared_memory->mutex_dispatcher);

        sem_wait(shared_memory->sem_servers);

        char temp[BUFSIZE];
        int possivel = 0;

        retirar(&MQ, &received_msg);
        for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
            if (possivel == 1)
                break;
            sem_wait(shared_memory->sem_performace);
            if (shared_memory->mode_cpu == 1) { // Modo Normal
                sem_post(shared_memory->sem_performace);

                sem_wait(shared_memory->sem_SM);
                if (servers[i].es_ativo == 0) {
                    sem_post(shared_memory->sem_SM);
                    sem_wait(shared_memory->sem_fila);
                    printf("DEBUG: passou while, %d | %d | %d | %d | n_tarefas: %d\n", shared_memory->Num_es_ativos, servers[0].es_ativo, servers[1].es_ativo, servers[2].es_ativo, shared_memory->n_tarefas);
                    sem_post(shared_memory->sem_fila);
                    // TODO:

                    if (received_msg.num_instrucoes / servers[i].mips1 < received_msg.max_tempo) { // FIXME: verficar esta condicao
                        int tempo_espera = time(NULL) - received_msg.tempo_chegada;
                        shared_memory->tempo_medio += tempo_espera;
                        if (write(servers[i].fd[WRITE], &received_msg, sizeof(Task)) == -1) {
                            perror("Erro ao escrever no pipe:");
                        }
                        sem_wait(shared_memory->sem_fila);
                        shared_memory->n_tarefas--;
                        pthread_cond_signal(&shared_memory->cond_monitor);
                        sem_post(shared_memory->sem_fila);
                        possivel = 1;
                        break;
                    }
                }

            } else if (shared_memory->mode_cpu == 2) { // Modo HP
                sem_post(shared_memory->sem_performace);
                sem_wait(shared_memory->sem_SM);
                if (servers[i].es_ativo < 2) {
                    sem_post(shared_memory->sem_SM);
                    sem_wait(shared_memory->sem_fila);
                    printf("DEBUG: passou while, %d | %d | %d | %d | n_tarefas: %d\n", shared_memory->Num_es_ativos, servers[0].es_ativo, servers[1].es_ativo, servers[2].es_ativo, shared_memory->n_tarefas);
                    sem_post(shared_memory->sem_fila);
                    if ((received_msg.num_instrucoes / servers[i].mips1 < received_msg.max_tempo ) || (received_msg.num_instrucoes / servers[i].mips2 < received_msg.max_tempo)) {
                        int tempo_espera = time(NULL) - received_msg.tempo_chegada;
                        // sem_wait(shared_memory->sem_estatisticas);
                        shared_memory->tempo_medio += tempo_espera;
                        // shared_memory->count+=1;
                        // sem_post(shared_memory->sem_estatisticas);
                        if (write(servers[i].fd[WRITE], &received_msg, sizeof(Task)) == -1) {
                            perror("Erro ao escrever no pipe:");
                        }
                        sem_wait(shared_memory->sem_fila);
                        shared_memory->n_tarefas--;
                        pthread_cond_signal(&shared_memory->cond_monitor);
                        sem_post(shared_memory->sem_fila);
                        possivel = 1;
                        break;
                    }
                } else
                    continue;
            }
            if (possivel == 0 && i == shared_memory->EDGE_SERVER_NUMBER) {
                char mensagem[200];
                snprintf(mensagem, 200, "Tempo insuficiente para executar a tarefa %d", received_msg.idTarefa);
                sem_wait(shared_memory->sem_fila);
                shared_memory->n_tarefas--;
                pthread_cond_signal(&shared_memory->cond_monitor);
                sem_post(shared_memory->sem_fila);
                sem_post(shared_memory->sem_servers);
                log_msg(mensagem, 0);
            }
        }
        sleep(1); // DEBUG: desbugar a vm necessita deste sleep ns pq
    }

    pthread_exit(NULL);
}

// Funcao encarregue de executar as tarefas de cada E-Server
void server(int i) { // TODO: recebe por um unamed pipe a tarefa a executar e se tiver em modo performace divide a tarefa pelas 2 threads,se tiver em modo 1 executa-a com 1 thread!
    // Configurar o ES com os dados obtidos do ES
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
        // Verificar se o ES vai entrar em manutencao
        memset(mensagem, 0, 200);
        priority_msg MQ_msg;
        if (msgrcv(MQid, &MQ_msg, sizeof(priority_msg), i, IPC_NOWAIT) != -1) {
            printf("DEBUG: ENTREI EM MANUTENCAO, ESPERANDO SERVER ACABAR TAREFA! %d \n", i + 1);
            pthread_mutex_lock(&shared_memory->mutex_manutencao);
            while (servers[i].es_ativo > 0) {
                printf("[SERVER_%d] espera while pthread_cond\n", i + 1);
                pthread_cond_wait(&shared_memory->cond_manutencao, &shared_memory->mutex_manutencao);
            }
            pthread_mutex_unlock(&shared_memory->mutex_manutencao);
            sem_wait(shared_memory->sem_performace);

            sem_post(shared_memory->sem_performace);
            sem_wait(shared_memory->sem_SM);

            shared_memory->Num_es_ativos += 2;
            servers[i].es_ativo = 2;
            servers[i].cpu_ativo[0] = 1;
            servers[i].cpu_ativo[1] = 1;

            sem_post(shared_memory->sem_SM);
            sem_post(shared_memory->sem_servers);

            servers[i].em_manutencao = 1;
            memset(mensagem, 0, 200);
            snprintf(mensagem, 200, "O SERVER_%d vai entrar em manutencao", i + 1);
            log_msg(mensagem, 0);

            // printf("\nSERVER%d RECEBEU SLEEP= %d\n\n", i + 1, MQ_msg.temp_man);// DEBUG: VER ESTE TEMPO QUE ESTA A RECEBER!!
            sleep(MQ_msg.temp_man);

            memset(mensagem, 0, 200);
            snprintf(mensagem, 200, "O SERVER_%d terminou a manutencao", i + 1);
            log_msg(mensagem, 0);
            sem_wait(shared_memory->sem_performace); // QUESTION: para que e que servem estes dois?

            // TODO: secalhar aqui podemos aproveitar o codigo comum aos dois e meter antes
            sem_post(shared_memory->sem_performace); // QUESTION: para que e que servem estes dois?
            sem_wait(shared_memory->sem_SM);
            servers[i].es_ativo = 0;

            shared_memory->Num_es_ativos -= 2;
            servers[i].cpu_ativo[0] = 0;
            servers[i].cpu_ativo[1] = 0;

            servers[i].em_manutencao = 0;
            sem_post(shared_memory->sem_SM);
            pthread_cond_signal(&shared_memory->cond_dispatcher);
        }

        // Recebe as tarefas enviadas pelo dispatcher
        Task tarefa; // LER PIPE
        if (read(servers[i].fd[READ], &tarefa, sizeof(Task)) == -1) {
            perror("Erro ao ler do pipe");
            continue;
        } else {
            printf("[SERVER_%d]  reveived, Task_id: %d number of requests: %d , max time: %d\n",
                   i + 1, tarefa.idTarefa, tarefa.num_instrucoes, tarefa.max_tempo);
        }

        // FIXME:
        aux->idTarefa = tarefa.idTarefa;
        aux->num_instrucoes = tarefa.num_instrucoes;
        sem_wait(shared_memory->sem_performace);

        if (shared_memory->mode_cpu == 1) { // Modo Normal
            sem_post(shared_memory->sem_performace);
            memset(mensagem, 0, 200);
            strcpy(aux->nome_server, servers[i].nome);
            aux->mips_vcpu = servers[i].mips1;

            aux->n_vcpu = 1;
            sem_wait(shared_memory->sem_SM);
            shared_memory->Num_es_ativos++;
            servers[i].es_ativo++;
            servers[i].cpu_ativo[aux->n_vcpu - 1] = 1;
            sem_post(shared_memory->sem_SM);
            sem_post(shared_memory->sem_servers);

            pthread_create(&servers[i].vCPU[aux->n_vcpu - 1], NULL, vCPU_routine, (void *)aux);

        } else if (shared_memory->mode_cpu == 2) { // Modo HighPerformance
            sem_post(shared_memory->sem_performace);
            memset(mensagem, 0, 200);
            strcpy(aux->nome_server, servers[i].nome);

            if (servers[i].cpu_ativo[0] == 0) {
                aux->mips_vcpu = servers[i].mips1;
                aux->n_vcpu = 1;
            } else if (servers[i].cpu_ativo[1] == 0) {
                aux->mips_vcpu = servers[i].mips2;
                aux->n_vcpu = 2;
            }
            sem_wait(shared_memory->sem_SM);
            shared_memory->Num_es_ativos++;
            servers[i].es_ativo++;
            servers[i].cpu_ativo[aux->n_vcpu - 1] = 1;
            sem_post(shared_memory->sem_SM);
            sem_post(shared_memory->sem_servers);

            pthread_create(&servers[i].vCPU[aux->n_vcpu - 1], NULL, vCPU_routine, (void *)aux);
        }
    }
}

void *p_scheduler(void *lista) { // gestão do escalonamento das tarefas
    // TODO: code here
    char temp[BUFSIZE];
    // Abrir pipe para ler
    int fd;
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

        // Decompor a string numa tarefa
        while (token != NULL) {
            // printf("DEBUG: TOKEN = %s\n", token);
            if (!strcmp(token, "EXIT") && kappa == 0) { // echo "EXIT" > TASK_PIPE
                // Acaba o programa
                SIGINT_HANDLER(1);
                break;
            } else if (!strcmp(token, "STATS") && kappa == 0) { // echo "STATS" > TASK_PIPE
                // Imprime as estatisticas
                SIGTSTP_HANDLER(1);
                break;
            } else {
                if (kappa == 0)
                    tarefa.idTarefa = atoi(token);

                else if (kappa == 1)
                    tarefa.num_instrucoes = atoi(token);
                else {
                    tarefa.max_tempo = atoi(token);
                    snprintf(temp, BUFSIZE, "[SCHEDULER] Read %d bytes: reveived, Task_id: %d number of requests: %d , max time: %d",
                             r, tarefa.idTarefa, tarefa.num_instrucoes, tarefa.max_tempo);
                    log_msg(temp, 0);

                    // TODO:  colocar a tarefa, tem que reorganizar a fila, ou seja, diminuir a prioridade e verificar se ainda
                    // tem tempo para executar as tarefas, diminuir a prioridade o tempo que ja passou desde a ultima vez que colocou!!
                    sem_wait(shared_memory->sem_fila);
                    reoorganizar(&MQ, time(NULL));
                    sem_post(shared_memory->sem_fila);
                    if (colocar(&MQ, tarefa, tarefa.max_tempo) == false) {
                        snprintf(temp, BUFSIZE, "[SCHEDULER]: A lista de tarefas esta cheia, tarefa %d ignorada", tarefa.idTarefa);
                        log_msg(temp, 0);
                        shared_memory->tarefas_descartadas++;
                    } else {
                        char temp[BUFSIZE];
                        sem_wait(shared_memory->sem_fila);
                        tarefa.tempo_chegada = time(NULL);
                        MQ.n_tarefas++;
                        shared_memory->n_tarefas++;
                        pthread_cond_signal(&shared_memory->cond_monitor);
                        snprintf(temp, BUFSIZE, "[SCHEDULER]: Tarefa inserida na fila nº %d", MQ.n_tarefas);
                        sem_post(shared_memory->sem_fila);
                        pthread_cond_signal(&shared_memory->cond_dispatcher);
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

    // Criação da thread dispatcher
    pthread_t dispatcher;
    pthread_create(&dispatcher, NULL, p_dispatcher, NULL); // Criação da thread dispatcher
    log_msg("Criação da thread dispatcher", 0);

    pthread_join(scheduler, NULL);
    pthread_join(dispatcher, NULL);
}

// Funcao encarregue de executar as tarefas do Edge Server
void *vCPU_routine(void *t) {
    argumentos aux = *(argumentos *)t;

    // Executar a tarefa
    int tempo_execucao = aux.num_instrucoes / aux.mips_vcpu;
    sleep(tempo_execucao);

    char mensagem[BUFSIZE];
    snprintf(mensagem, BUFSIZE, "SERVER_%d: Task %d completada", aux.ES_num + 1, aux.idTarefa);
    log_msg(mensagem, 0);

    sem_wait(shared_memory->sem_SM);
    servers[aux.ES_num].tarefas_executadas++;
    servers[aux.ES_num].es_ativo--;
    servers[aux.ES_num].cpu_ativo[aux.n_vcpu - 1] = 0;
    shared_memory->Num_es_ativos--;
    sem_post(shared_memory->sem_SM);

    pthread_cond_broadcast(&shared_memory->cond_manutencao);
    pthread_cond_signal(&shared_memory->cond_dispatcher);

    pthread_exit(NULL);
}