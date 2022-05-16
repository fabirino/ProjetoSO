// Eduardo Figueiredo 2020213717
// Fábio Santos 2020212310

#include "auxiliar.h"

// criar a message queue interna a la maneta !!
base lista_tarefas;

void *p_dispatcher(void *lista) { // distribuição das tarefas

    Task received_msg; // DEBUG:apenas testes!!!! IR PARA OS EDGE SERVERS !!!

    while (1) {

        // Verificar os ES que estao livres
        sem_wait(shared_memory->sem_SM);
        sem_wait(shared_memory->sem_performace);
        sem_wait(shared_memory->sem_fila);
        sem_wait(shared_memory->sem_manutencao);
        int count = 0;
        for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
            if (servers[i].em_manutencao == 0) {

                if (servers[i].cpu_ativo[0] == 0) {
                    count++;
                }
                if (servers[i].cpu_ativo[1] == 0) {
                    count++;
                }
            }
        }

        pthread_mutex_lock(&shared_memory->mutex_dispatcher);
        while (((shared_memory->mode_cpu == 1) && (shared_memory->Num_es_ativos >= shared_memory->EDGE_SERVER_NUMBER)) || (shared_memory->n_tarefas == 0) || ((shared_memory->mode_cpu == 2) && (count <= 0)) || shared_memory->exit == 1) {
            sem_post(shared_memory->sem_performace);
            sem_post(shared_memory->sem_SM);
            sem_post(shared_memory->sem_fila);
            sem_post(shared_memory->sem_manutencao);
            pthread_cond_wait(&shared_memory->cond_dispatcher, &shared_memory->mutex_dispatcher);
            sem_wait(shared_memory->sem_SM);
            sem_wait(shared_memory->sem_performace);
            sem_wait(shared_memory->sem_fila);
            sem_wait(shared_memory->sem_manutencao);
            count = 0;
            for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
                if (servers[i].em_manutencao == 0) {

                    if (servers[i].cpu_ativo[0] == 0) {
                        count++;
                    }
                    if (servers[i].cpu_ativo[1] == 0) {
                        count++;
                    }
                }
            }
        }
        if (!retirar(&lista_tarefas, &received_msg)) {
            sem_post(shared_memory->sem_performace);
            sem_post(shared_memory->sem_SM);
            sem_post(shared_memory->sem_fila);
            sem_post(shared_memory->sem_manutencao);
            continue;
        }

        sem_post(shared_memory->sem_performace);
        sem_post(shared_memory->sem_SM);
        sem_post(shared_memory->sem_fila);
        sem_post(shared_memory->sem_manutencao);
        pthread_mutex_unlock(&shared_memory->mutex_dispatcher);

        struct timeval s_time;
        gettimeofday(&s_time, NULL);

        float inter = (float)(s_time.tv_sec - received_msg.tempo_chegada.tv_sec);
        inter += (s_time.tv_usec - received_msg.tempo_chegada.tv_usec) * 1e-6;
        float max = received_msg.max_tempo - inter;

        if (inter < 1000000) {
            if (max <= 0) { // Retirar as mensagens cujo tempo de execucao ja expirou
                char mensagem[BUFSIZE];
                memset(mensagem, 0, BUFSIZE);
                snprintf(mensagem, BUFSIZE, "[DISPATCHER] a tarefa com id %d foi descartada pois ja nao tem tempo para ser executada ||%f!", received_msg.idTarefa, max);
                log_msg(mensagem, 0);
                sem_wait(shared_memory->sem_fila);
                lista_tarefas.n_tarefas--;
                shared_memory->n_tarefas--;
                shared_memory->tarefas_descartadas++;
                sem_post(shared_memory->sem_fila);
                continue;
            } else { // Diminuir os tempos de execucao
                received_msg.max_tempo = max;
            }
        }

        char temp[BUFSIZE];
        bool possivel = false;

        sem_wait(shared_memory->sem_servers);
        for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
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
                        struct timeval stop_time;
                        gettimeofday(&stop_time, NULL);
                        float tempo_espera = (float)(stop_time.tv_sec - received_msg.tempo_chegada.tv_sec);
                        tempo_espera += (stop_time.tv_usec - received_msg.tempo_chegada.tv_usec) * 1e-6;
                        printf("tempo_espera ->%f\n", tempo_espera);
                        sem_wait(shared_memory->sem_fila);
                        printf("DEBUG: passou while, %d | %d | %d | %d | n_tarefas: %d\n", shared_memory->Num_es_ativos, servers[0].es_ativo, servers[1].es_ativo, servers[2].es_ativo, shared_memory->n_tarefas);
                        if (lista_tarefas.n_tarefas <= 1 || tempo_espera > 1000000)
                            sem_post(shared_memory->sem_fila);
                        else {
                            sem_post(shared_memory->sem_fila);
                            sem_wait(shared_memory->sem_SM);
                            shared_memory->tempo_medio += tempo_espera;
                            sem_post(shared_memory->sem_SM);
                            printf("tempo espera!! -> %f\n", tempo_espera);
                        }
                        if (write(servers[i].fd[WRITE], &received_msg, sizeof(Task)) == -1) {
                            perror("Erro ao escrever no pipe:");
                        }
                        sem_wait(shared_memory->sem_fila);
                        shared_memory->n_tarefas--;
                        pthread_cond_signal(&shared_memory->cond_monitor);
                        sem_post(shared_memory->sem_fila);
                        possivel = true;
                        break;
                    }
                } else {
                    sem_post(shared_memory->sem_SM);
                }

            } else if (shared_memory->mode_cpu == 2) { // Modo HP //FIXME: BUGADO ATE AO PESCOÇO, MAS JA ESTEVE MAIS!!
                sem_post(shared_memory->sem_performace);
                sem_wait(shared_memory->sem_SM);

                if (servers[i].es_ativo < 2) {

                    sem_post(shared_memory->sem_SM);
                    if (received_msg.num_instrucoes / servers[i].mips1 < received_msg.max_tempo) {
                        struct timeval stop_time;
                        gettimeofday(&stop_time, NULL);
                        float tempo_espera = (float)(stop_time.tv_sec - received_msg.tempo_chegada.tv_sec);
                        tempo_espera += (stop_time.tv_usec - received_msg.tempo_chegada.tv_usec) * 1e-6;
                        printf("tempo_espera ->%f\n", tempo_espera);
                        sem_wait(shared_memory->sem_fila);
                        printf("DEBUG: passou while, %d | %d | %d | %d | n_tarefas: %d\n", shared_memory->Num_es_ativos, servers[0].es_ativo, servers[1].es_ativo, servers[2].es_ativo, shared_memory->n_tarefas);
                        if (lista_tarefas.n_tarefas <= 1 || tempo_espera > 1000000)
                            sem_post(shared_memory->sem_fila);
                        else {
                            sem_post(shared_memory->sem_fila);
                            sem_wait(shared_memory->sem_SM);
                            shared_memory->tempo_medio += tempo_espera;
                            sem_post(shared_memory->sem_SM);
                        }
                        // sem_wait(shared_memory->sem_estatisticas);
                        if (write(servers[i].fd[WRITE], &received_msg, sizeof(Task)) == -1) {
                            perror("Erro ao escrever no pipe:");
                        }
                        sem_wait(shared_memory->sem_fila);
                        shared_memory->n_tarefas--;
                        pthread_cond_signal(&shared_memory->cond_monitor);
                        sem_post(shared_memory->sem_fila);
                        possivel = true;
                        break;
                    }
                } else {
                    sem_post(shared_memory->sem_manutencao);
                    sem_post(shared_memory->sem_SM);
                }
            } else {
                sem_post(shared_memory->sem_performace);
            }
        }
        if (possivel == false) {
            char mensagem[200];
            snprintf(mensagem, 200, "Tempo insuficiente para executar a tarefa %d , max_temp = %f", received_msg.idTarefa, received_msg.max_tempo);
            sem_wait(shared_memory->sem_fila);
            shared_memory->n_tarefas--;
            pthread_cond_signal(&shared_memory->cond_monitor);
            sem_post(shared_memory->sem_fila);
            sem_post(shared_memory->sem_servers);
            log_msg(mensagem, 0);
        }
        sleep(0.4); // DEBUG: desbugar a vm necessita deste sleep ns pq
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

        sem_wait(shared_memory->sem_performace);
        if (msgrcv(MQid, &MQ_msg, sizeof(priority_msg), i + 1, IPC_NOWAIT) != -1 && shared_memory->mode_cpu == 1) {
            sem_post(shared_memory->sem_performace);
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
            sem_post(shared_memory->sem_SM);
            // sem_post(shared_memory->sem_servers);

            sem_wait(shared_memory->sem_manutencao);
            servers[i].em_manutencao = 1;
            sem_post(shared_memory->sem_manutencao);

            memset(mensagem, 0, 200);
            snprintf(mensagem, 200, "O SERVER_%d vai entrar em manutencao", i + 1);
            log_msg(mensagem, 0);

            // printf("\nSERVER%d RECEBEU SLEEP= %d\n\n", i + 1, MQ_msg.temp_man);// DEBUG: VER ESTE TEMPO QUE ESTA A RECEBER!!
            sleep(MQ_msg.temp_man);

            memset(mensagem, 0, 200);
            snprintf(mensagem, 200, "O SERVER_%d terminou a manutencao", i + 1);
            log_msg(mensagem, 0);
            sem_wait(shared_memory->sem_performace); // QUESTION: para que e que servem estes dois?

            sem_post(shared_memory->sem_performace); // QUESTION: para que e que servem estes dois?
            sem_wait(shared_memory->sem_SM);
            servers[i].es_ativo = 0;
            shared_memory->Num_es_ativos -= 2;
            sem_post(shared_memory->sem_SM);

            sem_wait(shared_memory->sem_manutencao);
            shared_memory->em_manutencao--;
            servers[i].em_manutencao = 0;
            servers[i].manutencao = 0;
            sem_post(shared_memory->sem_manutencao);
            pthread_mutex_lock(&shared_memory->mutex_dispatcher);
            pthread_cond_broadcast(&shared_memory->cond_dispatcher);
            pthread_cond_signal(&shared_memory->cond_maintenance);
            pthread_mutex_unlock(&shared_memory->mutex_dispatcher);
        } else {
            sem_post(shared_memory->sem_performace);
        }

        // Recebe as tarefas enviadas pelo dispatcher
        Task tarefa; // LER PIPE
        if (read(servers[i].fd[READ], &tarefa, sizeof(Task)) == -1) {
            perror("Erro ao ler do pipe");
            continue;
        } else {
            printf("[SERVER_%d]  reveived, Task_id: %d number of requests: %d , max time: %lf\n",
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
        memset(mensagem, 0, BUFSIZE);
        r = read(fd, &mensagem, sizeof(mensagem));
        if (r < 0)
            perror("Named Pipe");

        int kappa = 0;
        char *token;
        token = strtok(mensagem, ";");
        int teste = 0;

        // Verificar os sinais do echo
        if (strlen(mensagem) < 7) { // para nao confundir com as tarefas EXIT-5 STATS-6
            mensagem[strlen(mensagem) - 1] = '\0';
            if (!strcmp(mensagem, "EXIT")) { // echo "EXIT" > TASK_PIPE
                // Acaba o programa
                SIGINT_HANDLER(1);
            } else if (!strcmp(mensagem, "STATS")) { // echo "STATS" > TASK_PIPE
                // Imprime as estatisticas
                SIGTSTP_HANDLER(1);
            } else {
                char temp[BUFSIZE * 2];
                snprintf(temp, BUFSIZE * 2, "Comando errado => %s", mensagem);
                log_msg(temp, 0);
            }
        }

        // Decompor a string numa tarefa
        while (token != NULL) {
            if (kappa == 0)
                tarefa.idTarefa = atoi(token);
            else if (kappa == 1)
                tarefa.num_instrucoes = atoi(token);
            else {
                tarefa.max_tempo = atoi(token);
                snprintf(temp, BUFSIZE, "[SCHEDULER] Read %d bytes: reveived, Task_id: %d number of requests: %d , max time: %lf",
                         r, tarefa.idTarefa, tarefa.num_instrucoes, tarefa.max_tempo);
                log_msg(temp, 0);

                // TODO:  colocar a tarefa, tem que reorganizar a fila, ou seja, diminuir a prioridade e verificar se ainda
                // tem tempo para executar as tarefas, diminuir a prioridade o tempo que ja passou desde a ultima vez que colocou!!
                sem_wait(shared_memory->sem_fila);
                reoorganizar(&lista_tarefas);
                if (colocar(&lista_tarefas, tarefa) == false) {
                    sem_post(shared_memory->sem_fila);
                    snprintf(temp, BUFSIZE, "[SCHEDULER]: A lista de tarefas esta cheia, tarefa %d ignorada", tarefa.idTarefa);
                    log_msg(temp, 0);
                    shared_memory->tarefas_descartadas++;
                    break;
                } else {
                    char temp[BUFSIZE];
                    gettimeofday(&tarefa.tempo_chegada, NULL);
                    lista_tarefas.n_tarefas++;
                    shared_memory->n_tarefas++;
                    snprintf(temp, BUFSIZE, "[SCHEDULER]: Tarefa inserida na fila nº %d", lista_tarefas.n_tarefas);
                    sem_post(shared_memory->sem_fila);
                    pthread_cond_signal(&shared_memory->cond_monitor);
                    pthread_cond_broadcast(&shared_memory->cond_dispatcher);
                    log_msg(temp, 0);
                    break;
                }
            }
            kappa++;
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

    inicializar(&lista_tarefas);

    // Criar um processo para cada Edge Server
    char teste[100];
    memset(teste, 0, 100);
    pid_t piid;
    for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
        if ((piid = fork()) == 0) {
            servers[i].pid = getpid();
            snprintf(teste, 100, "Edge server %d arrancou", i + 1);
            log_msg(teste, 0);

            pthread_t thread;
            pthread_create(&thread, NULL, out, NULL);

            server(i);
            exit(0);
        }
    }

    // Criação da thread scheduler
    pthread_create(&shared_memory->scheduler, NULL, p_scheduler, NULL); // Criação da thread scheduler
    log_msg("Criação da thread scheduler", 0);

    // Criação da thread dispatcher
    pthread_create(&shared_memory->dispatcher, NULL, p_dispatcher, NULL); // Criação da thread dispatcher
    log_msg("Criação da thread dispatcher", 0);

    pthread_join(shared_memory->scheduler, NULL);
    pthread_join(shared_memory->dispatcher, NULL);
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
    pthread_cond_broadcast(&shared_memory->cond_dispatcher);

    pthread_exit(NULL);
}