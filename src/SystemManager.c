// Eduardo Figueiredo 2020213717
// Fábio Santos 2020212310

// ./offload_simulator configfile.txt

#include "auxiliar.h"

// TODO: semafro para aceder a dados da memoria partilhada!!
// TODO: protecoes no config file(verificar se é inteiro ou string)

int main(int argc, char *argv[]) {

    if (argc != 2) {
        erro("Parametros errados. Exemplo:\noffload_simulator <config_file>\n");
    }

    // Create shared memory
    shmid = shmget(IPC_PRIVATE, sizeof(SM), IPC_CREAT | 0766);
    // Attach shared memory
    shared_memory = (SM *)shmat(shmid, NULL, 0);

    // Inicializar semaforos;
    sem_unlink("SEM_MANUTENCAO");
    sem_unlink("SEM_TAREFAS");
    sem_unlink("SEM_FICHEIRO");
    sem_unlink("SEM_SM");
    sem_unlink("SEM_SERVERS");
    shared_memory->sem_manutencao = sem_open("SEM_MANUTENCAO", O_CREAT | O_EXCL, 0700, 1);
    shared_memory->sem_tarefas = sem_open("SEM_TAREFAS", O_CREAT | O_EXCL, 0700, 1);
    shared_memory->sem_ficheiro = sem_open("SEM_FICHEIRO", O_CREAT | O_EXCL, 0700, 1);
    shared_memory->sem_servers = sem_open("SEM_SERVERS", O_CREAT | O_EXCL, 0700, 1);
    // Semaforo para ler e escrever da Shared Memory
    shared_memory->sem_SM = sem_open("SEM_SM", O_CREAT | O_EXCL, 0700, 1);

    // Variavel de condicao e semaforo TODO: testando da para alterar a variavel de condicao noutro processo
    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&cattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&shared_memory->mutex_dispatcher, &mattr);
    pthread_cond_init(&shared_memory->cond_dispatcher, &cattr);
    pthread_mutex_init(&shared_memory->mutex_manutencao, &mattr);
    pthread_cond_init(&shared_memory->cond_manutencao, &cattr);

    log_msg("O programa iniciou", 1);

    // Read config file
    char path[20];
    strcpy(path, argv[1]);

    config(path);
    shared_memory->Num_es_ativos = 0;

    shmserversid = shmget(IPC_PRIVATE, sizeof(sizeof(Edge_Server) * shared_memory->EDGE_SERVER_NUMBER), IPC_CREAT | 0766);

    // Attach shared memory
    servers = (Edge_Server *)shmat(shmserversid, NULL, 0);
    createEdgeServers(path);

    // cria o named pipe se ainda nao existe
    if ((mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)) {
        perror("Cannot create named pipe: ");
        exit(0);
    }

    log_msg("O TASK_PIPE iniciou", 0);
    // #=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

    //  Create Message QUEUE
    if ((MQid = msgget(IPC_PRIVATE, IPC_CREAT | 0700)) == -1) {
        erro("Erro ao criar a Message Queue");
    }

    // Monitor =============================================================================
    if ((shared_memory->monitor_pid = fork()) == 0) {
        log_msg("O processo Monitor comecou", 0);

        // while (1) { // TODO: VARIAVEL DE CONDICAO PARA SABER QUE ENTROU UMA MENSAGEM OU SAIU PARA VERIFIVAR!!
        //     if (shared_memory->mode_cpu == 1) {
        //         int tempo = 5; // FIXME:
        //         if (shared_memory->n_tarefas >= shared_memory->QUEUE_POS * 0.8 && tempo >= shared_memory->MAX_WAIT) {
        //             log_msg("O Sistema entrou em High Performance", 0);
        //             shared_memory->mode_cpu = 2;
        //         }
        //     } else if (shared_memory->mode_cpu == 2) {
        //         if (shared_memory->n_tarefas <= shared_memory->QUEUE_POS * 0.2) {
        //             log_msg("O Sistema voltou ao modo Normal", 0);
        //             shared_memory->mode_cpu = 1;
        //         }
        //     }
        //     sleep(1); // Fica lento sem o sleep, entao so verifica a cada 1s
        // }

        exit(0);
    }
    // Task Manager ========================================================================
    if ((shared_memory->TM_pid = fork()) == 0) {
        log_msg("O processo Task Manager comecou", 0);

        // Catch Signals

        task_manager();

        // Catch Signals
        signal(SIGTSTP, SIGTSTP_HANDLER);
        signal(SIGINT, SIGINT_HANDLER);
        while (wait(NULL) > 0)
            ;

        exit(0);
    }

    // // Ignorar os sinais nos seguintes processos para nao haver prints duplicados
    // signal(SIGTSTP, SIG_IGN);
    // signal(SIGINT, SIG_IGN);

    // Maintenance Manager =================================================================
    if ((shared_memory->maintenance_pid = fork()) == 0) {
        log_msg("O processo Maintenance Manager comecou", 0);
        sleep(5);

        while (1) { // BUG: QUANDO COLOCA-SE ESTE WHILE FICA TODO LAGADO A VM!!!!!!! RESORVER NS COMO
            // TODO: MM tera de mandar uma msg pela MQ para entrar em STOPPED
            // como e que o edge server responde ao MM??????????

            time_t t;
            srand((unsigned)time(&t));

            int tempo = rand() % 5 + 1;

            sleep(tempo);

            int servidor;

            int count = 0;
            int existe = 0;
            int array[shared_memory->EDGE_SERVER_NUMBER];

            // criar um array so com os ES que nao estao em manutencao no momento
            for (int i = 0; i < shared_memory->EDGE_SERVER_NUMBER; i++) {
                if (servers[i].em_manutencao == 0) {
                    array[count++] = i;
                    existe = 1;
                }
            }
            // se existir ES ativos, escolhe um para entrar em manutencao
            if (existe && count <= shared_memory->EDGE_SERVER_NUMBER && count > 1) {
                // Escolher um servidor para entrar em manutencao
                servidor = array[rand() % count];
                servers[servidor].em_manutencao = 1;

                // detalhes da mensagem
                priority_msg my_msg;
                my_msg.priority = servidor;
                my_msg.temp_man = tempo;
                msgsnd(MQid, &my_msg, sizeof(priority_msg), 0);
                printf("manutencao server n_[%d]\n", servidor);

                // tempo = rand() % 5 + 1;

                // sleep(tempo);

                msgsnd(MQid, &my_msg, sizeof(priority_msg), 0);

                servers[servidor].manutencoes += 1;
            }
        }

        exit(0);
    }

    // Catch Signals
    // signal(SIGTSTP, SIGTSTP_HANDLER);
    // signal(SIGINT, SIGINT_HANDLER);

    while (wait(NULL) > 0)
        ;

    return 0;
}
