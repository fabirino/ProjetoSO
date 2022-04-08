#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

void erro(char *msg) {
    perror(msg);
    exit(1);
}

//devolve o tempo formatado
void time_now(char * string){
    time_t tempo = time(NULL);
    if(tempo == -1){
        erro("Erro na funcao time():");
    }
    
    struct tm *ptm = localtime(&tempo);

    if (ptm == NULL){
        erro("Erro na funcao localtime():");
    }

    snprintf(string, 10, "%02d:%02d:%02d ",ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}


// a funcao escreve no ficheiro log da primeira vez e nas proximas da append
void log_msg(char *msg, int first_time){
    char mensagem[100];
    time_now(mensagem);

    strcat(mensagem, msg);

    printf("%s\n", mensagem);

    if(first_time == 0){
        FILE *log = fopen("log.txt", "w");
        if(log == NULL){
            erro("Erro ao abrir o ficheiro");
        }
        fprintf(log, "%s",mensagem);
        fprintf(log, "\n");
    }else if(first_time == 1){
        FILE *log = fopen("log.txt", "a");
        if(log == NULL){
            erro("Erro ao abrir o ficheiro");
        }
        fprintf(log, "%s",mensagem);
        fprintf(log, "\n");
    }
}