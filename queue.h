#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include "config.h"

typedef struct Queue Queue;

int queue_enqueue(Queue *q, const char *id); /* 0 ok, !=0 erro */
int queue_dequeue(Queue *q, char *out, size_t out_size); /* 0 ok, !=0 erro */
int queue_is_full(const Queue *q);
int queue_contains(const Queue *q, const char *id);
void queue_print(const Queue *q);

/* criação / destruição para tipos opacos */
Queue* queue_create(int cap);
void   queue_destroy(Queue *q);

int queue_size(const Queue *q);
int queue_get_id_by_index(const Queue *q, int index, char *out, size_t out_size);

#endif