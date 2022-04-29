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
    int shmid = shmget(IPC_PRIVATE, sizeof(SM), IPC_CREAT | 0766);

    // Attach shared memory
    shared_memory = (SM *)shmat(shmid, NULL, 0);

    // Inicializar semaforos;
    sem_unlink("SEM_MANUTENCAO");
    sem_unlink("SEM_TAREFAS");
    sem_unlink("SEM_FICHEIRO");
    sem_unlink("SEM_SM");
    shared_memory->sem_manutencao = sem_open("SEM_MANUTENCAO", O_CREAT | O_EXCL, 0700, 1);
    shared_memory->sem_tarefas = sem_open("SEM_TAREFAS", O_CREAT | O_EXCL, 0700, 1);
    shared_memory->sem_ficheiro = sem_open("SEM_FICHEIRO", O_CREAT | O_EXCL, 0700, 1);
    // Semaforo para ler e escrever da Shared Memory
    shared_memory->sem_SM = sem_open("SEM_SM", O_CREAT | O_EXCL, 0700, 1);

    // Variavel de condicao e semaforo TODO: testando da para alterar a variavel de condicao noutro processo
    pthread_mutexattr_t mattr;
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_t cattr;
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&shared_memory->pthread_sem, &mattr);
    pthread_cond_init(&shared_memory->pthread_cond, &cattr);

    log_msg("O programa iniciou", 1);

    // cria o named pipe se ainda nao existe
    if ((mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)) {
        perror("Nao se pode criar o pipe:");
        exit(0);
    }
    log_msg("O TASK_PIPE iniciou", 0);

    // Read config file
    char path[20];
    strcpy(path, argv[1]);

    config(path);
    shared_memory->servers = (Edge_Server *)malloc(sizeof(Edge_Server) * shared_memory->EDGE_SERVER_NUMBER);
    createEdgeServers(path);
    // DEBUG:
    // for (int p = 0; p < shared_memory->EDGE_SERVER_NUMBER; p++) {
    //     printf("%s\n", shared_memory->servers[p].nome);
    // }

    // #=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

    // Catch Signals
    signal(SIGTSTP, SIGTSTP_HANDLER);
    signal(SIGINT, SIGINT_HANDLER);

    //  Create Message QUEUE
    if ((MQid = msgget(IPC_PRIVATE, IPC_CREAT | 0700)) == -1) {
        erro("Erro ao criar a Message Queue");
    }

    // Monitor =============================================================================
    if ((shared_memory->monitor_pid = fork()) == 0) {
        log_msg("O processo Monitor comecou", 0);

        // while(1){
        //     // FIXME: o tempo vai ser o tempo das tarefas, alterar mais tarde
        //     int tempo=5; // MAX_WAIT esta a 5, ou seja a condicao de baixo verifica-se sempre
        //     if(MQ->tam >= shared_memory->QUEUE_POS && tempo >= shared_memory->MAX_WAIT){

        //     }
        // }

        exit(0);
    }

    // Task Manager ========================================================================
    if ((shared_memory->TM_pid = fork()) == 0) {
        log_msg("O processo Task Manager comecou", 0);

        // criar a message queue interna a la maneta !!
        base *MQ;
        MQ->nos = (no_fila *)malloc(sizeof(no_fila) * shared_memory->QUEUE_POS);

        inicializar(MQ, shared_memory->QUEUE_POS);

        task_manager(MQ);

        while (wait(NULL) > 0)
            ;

        exit(0);
    }

    // Maintenance Manager =================================================================
    if ((shared_memory->maintenance_pid = fork()) == 0) {
        log_msg("O processo Maintenance Manager comecou", 0);

        // while(1){//BUG: QUANDO COLOCA-SE ESTE WHILE FICA TODO LAGADO A VM!!!!!!! RESORVER NS COMO
        //     // TODO: MM tera de mandar uma msg pela MQ para entrar em STOPPED
        //     // como e que o edge server responde ao MM??????????

        //     time_t t;
        //     srand((unsigned)time(&t));

        //     int tempo = random() % 5 + 1;

        //     int servidor = random() % shared_memory->EDGE_SERVER_NUMBER;

        //     int existe = 0;

        //      for (int i = 0; i<shared_memory->EDGE_SERVER_NUMBER; i++){
        //         if (shared_memory->servers[i].em_manutencao){
        //             existe = 1;
        //         }
        //     }
        //     if(!existe){
        //         shared_memory->servers[servidor].em_manutencao = 1;
        //         char temp[BUFSIZE];
        //         snprintf(temp, BUFSIZE, "O Edge Server %d entrou em manutencao", servidor + 1);
        //         //sleep(tempo);

        //         shared_memory->servers[servidor].manutencoes +=1;

        //         tempo = random() % 5 + 1;
        //         //sleep(tempo);
        //     }

        // }

        exit(0);
    }

    while (wait(NULL) > 0)
        ;

    return 0;
}
