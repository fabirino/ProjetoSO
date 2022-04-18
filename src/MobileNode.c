//Eduardo Figueiredo 2020213717
//Fábio Santos 2020212310

#include "auxiliar.h"

// mobile_node {nº pedidos a gerar} {intervalo entre pedidos em ms}
// {milhares de instruções de cada pedido} {tempo máximo para execução}

int main(int argc, char *argv[]) {

    if (argc != 5) {
        erro("Parametros errados. Exemplo:\nmobile_node <num pedidos> <intervalo entre pedidos (ms)> <MIPS por pedido> <tempo máximo de execução>\n");
    }

    MN *mobile_node;
    char *ponteiro;

    mobile_node->num_pedidos = (int)strtol(argv[1], &ponteiro, 10);
    mobile_node->intervalo_tempo = (int)strtol(argv[2], &ponteiro, 10);
    mobile_node->mips = (int)strtol(argv[3], &ponteiro, 10);
    mobile_node->max_tempo = (int)strtol(argv[4], &ponteiro, 10);

    printf("%d\n", mobile_node->num_pedidos);
    printf("%d\n", mobile_node->intervalo_tempo);
    printf("%d\n", mobile_node->mips);
    printf("%d\n", mobile_node->max_tempo);

    return 0;
}
