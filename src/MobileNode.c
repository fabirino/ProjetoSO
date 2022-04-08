#include "auxiliar.h"

typedef struct Mobile_Node {
    int num_pedidos;
    int intervalo_tempo;
    int mips;
    int max_tempo;

} MN;


int main(int argc, char *argv[]) {

    if (argc != 5) {
        erro("Parametros errados. Exemplo:\nmobile_node <num pedidos> <intervalo entre pedidos (ms)> <MIPS por pedido> <tempo máximo de execução>\n");
    }

    MN *mobile_node;
    char *ponteiro;
    mobile_node->num_pedidos = (int)strtol(argv[1], &ponteiro, 10);
    mobile_node->intervalo_tempo = (int)strtol(argv[2], &ponteiro, 10);
    mobile_node->mips = (int)strtol(argv[3], &ponteiro, 10);
    mobile_node->max_tempo = (int)strtol(argv[4], &ponteiro, 10);;

    printf("%d\n", mobile_node->num_pedidos);
    printf("%d\n", mobile_node->intervalo_tempo);
    printf("%d\n", mobile_node->mips);
    printf("%d\n", mobile_node->max_tempo);

    return 0;
}
