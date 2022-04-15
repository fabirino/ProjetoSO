#ifndef AUXILIAR_H
#define AUXILIAR_H

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>
#include <sys/shm.h>

typedef struct {
    char nome[50];
    int mips1;
    int mips2;
} Edge_Server;

typedef struct shared_memory {
    int QUEUE_POS;
    int MAX_WAIT;
    int EDGE_SERVER_NUMBER;
    Edge_Server *servers;
} SM;

typedef struct Mobile_Node {
    int num_pedidos;
    int intervalo_tempo;
    int mips;
    int max_tempo;

} MN;

void erro(char *msg);

void time_now(char * string);

void log_msg(char *msg, int first_time);

void config(char *path, SM *shared_memory);

void createEdgeServers(char *path, SM *shared_memory);

#endif