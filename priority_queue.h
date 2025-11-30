#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stddef.h>
#include "config.h"

typedef struct PriorityQueue PriorityQueue;

/* Prioridades válidas (1 = emergência ... 5 = não urgência) */
int pqueue_enqueue(PriorityQueue *pq, const char *id, int priority); /* 0 ok */
int pqueue_dequeue(PriorityQueue *pq, char *out, size_t out_size);   /* 0 ok */
int pqueue_is_full(const PriorityQueue *pq);
int pqueue_contains(const PriorityQueue *pq, const char *id);
int pqueue_size(const PriorityQueue *pq);
int pqueue_get_id_by_index(const PriorityQueue *pq, int index, char *out, size_t out_size);
int pqueue_get_priority_by_index(const PriorityQueue *pq, int index, int *out_priority);

PriorityQueue *pqueue_create(int cap);
void pqueue_destroy(PriorityQueue *pq);

#endif