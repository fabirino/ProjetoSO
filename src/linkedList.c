#include "auxiliar.h"

typedef struct  {
    int idTarefa;
    int num_instrucoes;
    int max_tempo;
    int prioridade;
    struct node *next;
}node;

typedef struct{
    node * firstnode;
    int n_tarefas;
}lista;


struct node *head = NULL;
struct node *sorted = NULL;

void push(int val) {
    /* allocate node */
    node *newnode = (node *)malloc(sizeof(node));
    newnode->data = val;
    /* link the old list off the new node */
    newnode->next = head;
    /* move the head to point to the new node */
    head = newnode;
}

void insert(lista ** mq,int num_instrucoes,int max_time,int prioridade,int id){
    node *newnode = (node *)malloc(sizeof(node));
    newnode->num_instrucoes = num_instrucoes;
    newnode->prioridade = prioridade;
    newnode->idTarefa = id;
    newnode->max_tempo = max_time;
    newnode->next = NULL;

    node *aux = (*mq)->firstnode;

    if(aux == NULL){
        (*mq)->firstnode = newnode;
    }
    else{
        while((aux->next!= NULL)){
            aux = aux->next;
        }
        aux->next = newnode;
    }
    shared_memory->n_tarefas++;

}

/*
 * function to insert a new_node in a list. Note that
 * this function expects a pointer to head_ref as this
 * can modify the head of the input linked list
 * (similar to push())
 */
void sortedInsert(node *newnode) {
    /* Special case for the head end */
    if (sorted == NULL || sorted->data >= newnode->data) {
        newnode->next = sorted;
        sorted = newnode;
    } else {
        node *current = sorted;
        /* Locate the node before the point of insertion
         */
        while (current->next != NULL && current->next->data < newnode->data) {
            current = current->next;
        }
        newnode->next = current->next;
        current->next = newnode;
    }
}

// function to sort a singly linked list
// using insertion sort
void insertionsort() {

    node *current = head;

    // Traverse the given linked list and insert every
    // node to sorted
    while (current != NULL) {

        // Store next for next iteration
         node *next = current->next;

        // insert current in sorted linked list
        sortedInsert(current);

        // Update current
        current = next;
    }
    // Update head to point to sorted linked list
    head = sorted;
}

/* Function to print linked list */
void printlist( node *head) {
    while (head != NULL) {
        printf("%d->", head->data);
        head = head->next;
    }
    printf("NULL");
}

// Driver program to test above functions
int main() {

    push(5);
    push(20);
    push(4);
    push(3);
    push(30);

    printf("Linked List before sorting:\n");
    printlist(head);
    printf("\n");

    insertionsort(head);

    printf("Linked List after sorting:\n");
    printlist(head);
}