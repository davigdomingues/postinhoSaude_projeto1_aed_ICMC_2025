#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stddef.h>
#include "config.h"

typedef struct PriorityQueue PriorityQueue;

/* Fila de prioridades (1 = emergência .. 5 = não urgência)
 * - Desempate por ordem de chegada (seq mais antigo primeiro).
 * - Acesso por índice retorna snapshot ordenado (não modifica o heap).
 * - io.c persiste itens da fila com prioridades.
 */

/* Enfileira um paciente com prioridade.
 * Parâmetros: pq (fila), id (string do paciente), priority (1..5).
 * Retorno: 0 em sucesso; -1 se fila cheia, id duplicado, prioridade inválida ou ponteiro nulo.
 */
int pqueue_enqueue(PriorityQueue *pq, const char *id, int priority); /* 0 ok */

/* Desenfileira o próximo paciente (maior prioridade / mais antigo no empate).
 * Parâmetros: pq (fila), out (buffer para id), out_size (tamanho do buffer).
 * Retorno: 0 em sucesso; -1 se fila vazia ou parâmetros inválidos.
 * Observação: copia o id para out e remove-o do conjunto de membros.
 */
int pqueue_dequeue(PriorityQueue *pq, char *out, size_t out_size);   /* 0 ok */

/* Predicado: retorna 1 se a fila está cheia (size >= cap); 0 caso contrário. */
int pqueue_is_full(const PriorityQueue *pq);

/* Consulta: retorna 1 se id está presente na fila; 0 caso contrário. */
int pqueue_contains(const PriorityQueue *pq, const char *id);

/* Tamanho atual da fila (número de itens). */
int pqueue_size(const PriorityQueue *pq);

/* Acesso por índice lógico (snapshot ordenado por prioridade/seq).
 * Parâmetros: pq, index (0..size-1), out (buffer para id), out_size.
 * Retorno: 0 em sucesso; -1 se índice inválido ou parâmetros inválidos.
 * Observação: não altera o heap interno.
 */
int pqueue_get_id_by_index(const PriorityQueue *pq, int index, char *out, size_t out_size);

/* Obtém a prioridade pelo índice lógico (snapshot ordenado).
 * Parâmetros: pq, index (0..size-1), out_priority (ponteiro para int).
 * Retorno: 0 em sucesso; -1 se índice inválido ou parâmetros inválidos.
 */
int pqueue_get_priority_by_index(const PriorityQueue *pq, int index, int *out_priority);

/* Criação: aloca fila com capacidade cap (1..WAIT_CAP).
 * Retorno: ponteiro válido em sucesso; NULL em erro (memória/capacidade inválida).
 */
PriorityQueue *pqueue_create(int cap);

/* Destruição: libera heap e tabela de membros. Aceita NULL (noop). */
void pqueue_destroy(PriorityQueue *pq);

#endif