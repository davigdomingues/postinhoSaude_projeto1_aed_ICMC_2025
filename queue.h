#include "config.h"

#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue_ Queue;

Queue* queue_create();
int queue_insert(Queue *q, char id[MAX_ID_LEN + 1]);
char* queue_remove(Queue *q);
char* queue_front(Queue *q);
int queue_size(Queue *q);
int queue_empty(Queue *q);
int queue_full(Queue *q);

#endif