#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "auxiliar.h"

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
void log_msg(char *msg, SM *shared_memory, int first_time){
    char mensagem[100];
    time_now(mensagem);

    strcat(mensagem, msg);

    printf("%s\n", mensagem);

    sem_wait(shared_memory->sem_ficheiro);

    if(first_time == 1){
        FILE *log = fopen("log.txt", "w");
        if(log == NULL){
            erro("Erro ao abrir o ficheiro");
        }
        fprintf(log, "%s",mensagem);
        fprintf(log, "\n");
    }else if(first_time == 0){
        FILE *log = fopen("log.txt", "a");
        if(log == NULL){
            erro("Erro ao abrir o ficheiro");
        }
        fprintf(log, "%s",mensagem);
        fprintf(log, "\n");
    }

    sem_post(shared_memory->sem_ficheiro);
}

// funcao que le o ficheiro de configuracao
void config(char *path, SM *shared_memory){ 
    FILE * fich = fopen(path, "r");
    assert(fich);

    char line[20];

    fscanf(fich, "%s", line);
    shared_memory->QUEUE_POS=atoi(line);

    fscanf(fich, "%s", line);
    shared_memory->MAX_WAIT = atoi(line);

    fscanf(fich, "%s", line);
    shared_memory->EDGE_SERVER_NUMBER = atoi(line);
    fclose(fich);
}

// funcao que cria os edge servers com base nos dados obtidos no ficheiro config
void createEdgeServers(char *path, SM *shared_memory) {

    FILE * fich = fopen(path, "r");
    assert(fich);

    char line[20];
    const char sep[2] = ",";

    int num = 0;
    int i = 0;
    char *ptr;
    while (fscanf(fich, "%[^\n] ", line) != EOF) {
        if(num > 2){
            char *token;
            token = strtok(line, sep);
            int n = 0;
            while (token != NULL) {
                if (n == 0){
                    strcpy(shared_memory->servers[i].nome, token);
                    n++;
                }

                else if (n == 1){
                    int ret ;
                    ret = (int) strtol(token, &ptr, 10);
                    shared_memory->servers[i].mips1 = ret;
                    n++;
                }
                else if (n == 2){
                    int ret;
                    ret = (int) strtol(token, &ptr, 10);
                    shared_memory->servers[i].mips2 = ret;
                    n++;
                }

                token = strtok(NULL, sep);
            }
            i++;
        }
        num++;
    }
    fclose(fich);
}

// Funcao encarregue de executar as tarefas do Edge Server
void *function(void *t){
    int my_id = *((int *)t);

}

// Funcao que trata do CTRL-Z (imprime as estatisticas)
void SIGTSTP_HANDLER(int signum){


}

// Funcao que trata do CTRL-C (termina o programa)
void SIGINT_HANDLER(int signum){ // TODO: como passar a shared memory por parametros nesta funcao??

    

    // Esperar que as threads dos Edge Servers terminem
    // for(int i = 0; i< shared_memory->EDGE_SERVER_NUMBER ;i++){
	// 	pthread_join(shared_memory->servers[i].vCPU1,NULL);
	// 	pthread_join(shared_memory->servers[i].vCPU2,NULL);
    // }

    // Fechar os semaforos
    // sem_close(shared_memory->sem_manutencao);
    // sem_close(shared_memory->sem_tarefas);
    // sem_close(shared_memory->sem_ficheiro);
    sem_unlink("SEM_MANUTENCAO");
    sem_unlink("SEM_TAREFAS");
    sem_unlink("SEM_FICHEIRO");


    // log_msg("O programa terminou\n", shared_memory, 0);
}
