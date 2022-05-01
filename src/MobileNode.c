// Eduardo Figueiredo 2020213717
// Fábio Santos 2020212310

#include "auxiliar.h"

// mobile_node {nº pedidos a gerar} {intervalo entre pedidos em ms}
// {milhares de instruções de cada pedido} {tempo máximo para execução}

int main(int argc, char *argv[]) {

    if (argc != 5) {
        erro("Parametros errados. Exemplo:\nmobile_node <num pedidos> <intervalo entre pedidos (ms)> <MIPS por pedido> <tempo máximo de execução>\n");
    }
    // Opens the pipe for writing
    int fd;
    if ((fd = open(PIPE_NAME, O_WRONLY)) < 0) {
        perror("Erro ao abrir o Pipe ");
        exit(0);
    }

    /* Intializes random number generator */
    time_t t;
    srand((unsigned)time(&t));

    int idTarefa;
    int num_instrucoes = atoi(argv[1]); //QUESTION: nao temos de multiplicar o numero de pedidos por 1000?
    int max_tempo = atoi(argv[4]);

    int intervalo_tempo = atoi(argv[2]); //QUESTION: e aqui multiplicar por 0.001 uma vez que sao em milisegundos
    int mips = atoi(argv[3]);

    // ID tarefa; Nº de instruções (em milhares); Tempo máximo para execução
    char mensagem[BUFSIZE];
    for (int i = 0; i < num_instrucoes; i++) {
        idTarefa = random(); // TODO: ?????????
        memset(mensagem,0,BUFSIZE);
        snprintf(mensagem, BUFSIZE, "%d", idTarefa);
        strcat(mensagem, ";");
        strcat(mensagem, argv[3]);
        strcat(mensagem, ";");
        strcat(mensagem, argv[4]);
        
        write(fd, &mensagem, sizeof(mensagem));
        sleep(intervalo_tempo);
    }

    //DEBUG:
    printf("%d\n", idTarefa);
    printf("%d\n", num_instrucoes);
    printf("%d\n", max_tempo);
    printf("%d\n", intervalo_tempo);
    printf("%d\n", mips);

    return 0;
}
