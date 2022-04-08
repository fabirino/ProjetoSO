#include "auxiliar.h"

typedef struct Mobile_Node{
    int num_pedidos;
    int intervalo_tempo;
    int mips;
    int max_tempo;

}MN;

int main(int argc, char *argv[]){

    if(argc!=4){
        erro("Parametros errados. Exemplo:\nmobile_node <num pedidos> <intervalo entre pedidos (ms)> <MIPS por pedido> <tempo máximo de execução>\n");
    }



    return 0;
}
