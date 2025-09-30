#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

typedef struct queue_ {
    char id[WAIT_CAP][MAX_ID_LEN + 1];
    int start;
    int end;
}Queue;

Queue* queue_create(){
    Queue *q;
    q->start = 0;
    q->end = 0;
    return q;
}

int queue_insert(Queue *q, char id[MAX_ID_LEN + 1]){
    strcpy(q->id[q->end], id);
    q->end++;
    if(q->end == WAIT_CAP) q->end = 0;
    // falta descobrir uma forma de checar se voltou certo
    return 1;
}

char* queue_remove(Queue *q){
    char *str = q->id[q->start];
    for (int i = 0; i <= MAX_ID_LEN; i++){
        q->id[q->start][i];
    }
    q->start--;
    if(q->start < 0) q->start = WAIT_CAP;
    return str;
}

char * queue_front(Queue *q){
    return q->id[0];
}

int queue_size(Queue *q){
    return q->end;
}

int queue_empty(Queue *q){
    if (q->end == 0) return 1; else return 0;
}

int queue_full(Queue *q){
    if (q->end == WAIT_CAP) return 1; else return 0;
}
