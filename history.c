#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"
#include "config.h"

/* Implementação de History (pilha de procedimentos).
 *
 * Observações:
 * - Não faz I/O em disco; os dados são persistidos via io.c quando necessário.
 */

struct History {
    char items[HIST_MAX][PROC_MAX_LEN + 1];
    int top;
};

/* Inicializa uma estrutura History (top = -1) */
void history_init(History *h) {
    if (!h) return;
    h->top = -1;
}

/* Liberacao logica do History (atualmente noop, mas mantido para extensibilidade) */
void history_free(History *h) {
    (void)h;
}

/* Empilha nova entrada:
   - verifica capacidade (HIST_MAX)
   - copia com strncpy garantindo terminação
   - incrementa top
   - retorna 0 sucesso, -1 se cheio/erro
*/
int history_push(History *h, const char *proc) {
    if (!h || !proc) return -1;
    if (h->top >= HIST_MAX - 1) return -1;
    h->top++;
    strncpy(h->items[h->top], proc, PROC_MAX_LEN);
    h->items[h->top][PROC_MAX_LEN] = '\0';
    return 0;
}

/* Remove topo:
   - copia conteudo do topo para 'out' (seguro)
   - zera a string no topo e decrementa top
   - retorna -1 se vazio
*/
int history_pop(History *h, char *out, size_t out_size) {
    if (!h || !out || out_size == 0) return -1;
    if (h->top < 0) return -1;
    strncpy(out, h->items[h->top], out_size - 1);
    out[out_size - 1] = '\0';
    h->items[h->top][0] = '\0';
    h->top--;
    return 0;
}

/* Copia (sem remover) a entrada no indice 'idx' para out. Retorna 0 sucesso, -1 erro */
int history_get_by_index(const History *h, int idx, char *out, size_t out_size) {
    if (!h || !out || out_size == 0) return -1;
    if (idx < 0 || idx > h->top) return -1;
    strncpy(out, h->items[idx], out_size - 1);
    out[out_size - 1] = '\0';
    return 0;
}

/* Cria/destroi History:
   - history_create encapsula malloc + history_init
   - history_destroy faz cleanup possivel (history_free) e free
*/
History *history_create(void) {
    History *h = (History *)malloc(sizeof(History));
    if (!h) return NULL;
    history_init(h);
    return h;
}

void history_destroy(History *h) {
    if (!h) return;
    history_free(h);
    free(h);
}

/* Predicados e utilitarios: is_full, is_empty, size, top, inspect (debug) */
bool history_is_full(const History *h) {
    if (!h) return false;
    return (h->top >= HIST_MAX - 1);
}

bool history_is_empty(const History *h) {
    if (!h) return true;
    return (h->top < 0);
}

int history_size(const History *h) {
    if (!h) return 0;
    return (h->top + 1);
}

const char *history_top(const History *h) {
    if (!h || h->top < 0) return NULL;
    return h->items[h->top];
}

/* Inspecao para debug: imprime todas as entradas do historico */
void history_inspect(History *h) {
    if (!h) return;
    printf("Historico (top = %d):\n", h->top);
    for (int i = 0; i <= h->top; ++i)
        printf("%d: %s\n", i, h->items[i]);
}