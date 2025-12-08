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

    - Usado por PatientTree para associar histórico por paciente.
    - Operações: push/pop/is_full/is_empty/size/top/get_by_index.
    - Persistência é feita por io.c via wrappers ptree_history_* (não dentro deste módulo).
*/

#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

/* Nota:
 * - History é um TAD em memória (pilha de procedimentos) opaco externamente.
 * - Uso externo deve ocorrer via history_create/history_destroy e operações públicas
 *   de push/pop/size/is_full/get_by_index.
 */

typedef struct History History;

/* Empilha nova entrada no histórico.
 * Parâmetros: h (histórico), proc (string de procedimento).
 * Retorno: 0 em sucesso; -1 se cheio ou parâmetros inválidos.
 */
int history_push(History *h, const char *proc);

/* Desempilha a última entrada e copia para out.
 * Parâmetros: h, out (buffer), out_size (tamanho do buffer).
 * Retorno: 0 em sucesso; -1 se vazio ou parâmetros inválidos.
 */
int history_pop(History *h, char *out, size_t out_size);

/* Predicados: cheio/vazio. */
bool history_is_full(const History *h);
bool history_is_empty(const History *h);

/* Tamanho atual (número de itens). */
int history_size(const History *h);

/* Ponteiro para o topo (somente leitura); NULL se vazio. */
const char *history_top(const History *h);

/* Inspeção (depuração): imprime itens via util_printf. */
void history_inspect(History *h);

/* Leitura de um item do histórico por índice sem remover:
 * - Copia a entrada 'idx' para out (out_size incl. terminador).
 * - Retorna 0 em sucesso, -1 em erro (índice inválido / ponteiro nulo).
 */
int history_get_by_index(const History *h, int idx, char *out, size_t out_size);

/* Ciclo de vida do TAD History. */
/* Cria histórico com capacidade vinda da configuração dinâmica. */
History *history_create(void);
/* Cria histórico com capacidade explícita (>=1). */
History *history_create_with_cap(int cap);
void      history_destroy(History *h);

#endif