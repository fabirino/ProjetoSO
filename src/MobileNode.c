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

    int idTarefa = random();// TODO: ?????????
    int num_pedidos = atoi(argv[1]);
    int max_tempo = atoi(argv[4]);

    char mensagem[BUFSIZE];
    snprintf(mensagem,BUFSIZE,"%d",idTarefa);
    strcat(mensagem,";");
    strcat(mensagem,argv[1]);
    strcat(mensagem,";");
    strcat(mensagem,argv[4]);

    int intervalo_tempo = atoi(argv[2]);
    int mips = atoi(argv[3]);

    // ID tarefa; Nº de instruções (em milhares); Tempo máximo para execução
    write(fd, &mensagem,sizeof(mensagem));

    printf("%d\n", idTarefa);
    printf("%d\n", num_pedidos);
    printf("%d\n", max_tempo);
    printf("%d\n", intervalo_tempo);
    printf("%d\n", mips);

    return 0;
}
