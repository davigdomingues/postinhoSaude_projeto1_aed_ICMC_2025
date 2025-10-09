/*
Explicação:
    history_init: Inicializa a TAD histórico.
    history_free: Apaga o TAD histórico.
    history_push: Adiciona um item no final do histórico.
    history_pop: Remove o último item do histórico.
    history_is_full: Verifica se o histórico está cheio.
    history_is_empty: Verifica se o histórico está vazio.
    history_size: Verifica o tamanho do histórico.
    history_top: Verifica o último item do histórico;
    history_inspect: Lê o histórico.
*/

#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

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

#endif
