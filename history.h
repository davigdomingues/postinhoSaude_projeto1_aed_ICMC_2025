#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

/* Nota:
 * - History é um TAD em memória (pilha de procedimentos). 
 * - A serialização do conteúdo do histórico é feita por io.c através de funções de wrapper (plist_history_*).
 */

typedef struct History History;

void history_init(History *h);
void history_free(History *h);

int history_push(History *h, const char *proc);

int history_pop(History *h, char *out, size_t out_size);

bool history_is_full(const History *h);
bool history_is_empty(const History *h);
int history_size(const History *h);

const char *history_top(const History *h);

void history_inspect(History *h);

/* Leitura de um item do histórico por índice sem remover:
 * - Copia a entrada 'idx' para out (out_size incl. terminador).
 * - Retorna 0 em sucesso, -1 em erro (índice inválido / ponteiro nulo).
 */
int history_get_by_index(const History *h, int idx, char *out, size_t out_size);

History *history_create(void);
void      history_destroy(History *h);

#endif