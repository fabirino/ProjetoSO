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
        perror("Cannot open pipe for writing: ");
        exit(0);
    }

    MN mobile_node;

    mobile_node.num_pedidos = atoi(argv[1]);
    mobile_node.intervalo_tempo =  atoi(argv[2]);
    mobile_node.mips =  atoi(argv[3]);
    mobile_node.max_tempo =  atoi(argv[4]);
    
    write(fd, &mobile_node, sizeof(MN));

    printf("%d\n", mobile_node.num_pedidos);
    printf("%d\n", mobile_node.intervalo_tempo);
    printf("%d\n", mobile_node.mips);
    printf("%d\n", mobile_node.max_tempo);

    return 0;
}
