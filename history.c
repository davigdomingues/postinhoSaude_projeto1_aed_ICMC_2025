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

void history_init(History *h) {
    if (!h) return;
    h->top = -1;
}

void history_free(History *h) {
    (void)h;
}

int history_push(History *h, const char *proc) {
    if (!h || !proc) return -1;
    if (h->top >= HIST_MAX - 1) return -1;
    h->top++;
    strncpy(h->items[h->top], proc, PROC_MAX_LEN);
    h->items[h->top][PROC_MAX_LEN] = '\0';
    return 0;
}

int history_pop(History *h, char *out, size_t out_size) {
    if (!h || !out || out_size == 0) return -1;
    if (h->top < 0) return -1;
    strncpy(out, h->items[h->top], out_size - 1);
    out[out_size - 1] = '\0';
    h->items[h->top][0] = '\0';
    h->top--;
    return 0;
}

int history_get_by_index(const History *h, int idx, char *out, size_t out_size) {
    if (!h || !out || out_size == 0) return -1;
    if (idx < 0 || idx > h->top) return -1;
    strncpy(out, h->items[idx], out_size - 1);
    out[out_size - 1] = '\0';
    return 0;
}

/* Cria (aloca + inicializa) um History opaco */
History *history_create(void) {
    History *h = (History *)malloc(sizeof(History));
    if (!h) return NULL;
    history_init(h);
    return h;
}

/* Destroi (libera internamente e free) um History criado por history_create */
void history_destroy(History *h) {
    if (!h) return;
    history_free(h);
    free(h);
}

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

void history_inspect(History *h) {
    if (!h) return;
    printf("Histórico (top=%d):\n", h->top);
    for (int i = 0; i <= h->top; ++i)
        printf("%d: %s\n", i, h->items[i]);
}