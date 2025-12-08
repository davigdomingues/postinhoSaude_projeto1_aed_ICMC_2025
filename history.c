#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"
#include "config.h"
#include "util.h"

/* Implementação de History (pilha de procedimentos, capacidade dinâmica).
 * - Persistência é responsabilidade de io.c (via ptree_history_*).
 * - util_printf é usado para inspeção com suporte a UTF-8.
 * - Capacidade inicial vem de config_get_hist_max() ou parâmetro explícito.
 */
struct History {
    char *items; /* buffer linear: cap * (PROC_MAX_LEN+1) */
    int cap; /* capacidade (número de entradas) */
    int top; /* índice do topo (inicial -1) */
};

/* Helpers para indexar no buffer linear */
static inline char *hist_at(History *h, int idx) {
    return h->items + (size_t)idx * (PROC_MAX_LEN + 1);
}

static void history_init(History *h, int cap) {
    if (!h) 
        return;

    h->cap = (cap > 0) ? cap : config_get_hist_max();
    h->top = -1;
    h->items = (char *)malloc((size_t)h->cap * (PROC_MAX_LEN + 1));
    
    if (!h->items) {
        h->cap = 0;
        h->top = -1;
    } 
    
    else {
        /* zera conteúdo para segurança */
        memset(h->items, 0, (size_t)h->cap * (PROC_MAX_LEN + 1));
    }
}

static void history_free(History *h) {
    if (!h) 
        return;

    free(h->items);
    
    h->items = NULL;
    h->cap = 0;
    h->top = -1;
}

/* Empilha nova entrada com checagem de capacidade dinâmica
   - verifica capacidade
   - copia com strncpy garantindo terminação
   - incrementa top
   - retorna 0 sucesso, -1 se cheio/erro
*/

int history_push(History *h, const char *proc) {
    if (h == NULL || proc == NULL || h->cap <= 0)
        return -1;

    if (h->top >= (h->cap - 1))
        return -1;

    h->top++;
    char *slot = hist_at(h, h->top);
    strncpy(slot, proc, PROC_MAX_LEN);
    slot[PROC_MAX_LEN] = '\0';
    return 0;
}

/* Remove topo:
   - copia conteudo do topo para 'out' (seguro)
   - zera a string no topo e decrementa top
   - retorna -1 se vazio
*/
int history_pop(History *h, char *out, size_t out_size) {
    if (h == NULL || out == NULL || out_size == 0 || h->cap <= 0)
        return -1;

    if (h->top < 0)
        return -1;

    char *slot = hist_at(h, h->top);
    strncpy(out, slot, out_size - 1);
    out[out_size - 1] = '\0';
    slot[0] = '\0';
    h->top--;
    return 0;
}

/* Copia (sem remover) a entrada no indice 'idx' para out. Retorna 0 sucesso, -1 erro */
int history_get_by_index(const History *h, int idx, char *out, size_t out_size) {
    if (h == NULL || out == NULL || out_size == 0 || h->cap <= 0)
        return -1;

    if (idx < 0 || idx > h->top)
        return -1;

    const char *slot = h->items + (size_t)idx * (PROC_MAX_LEN + 1);
    strncpy(out, slot, out_size - 1);
    out[out_size - 1] = '\0';
    return 0;
}

/* Cria/destroi History:
   - history_create encapsula malloc + history_init
   - history_destroy faz cleanup possivel (history_free) e free
*/
History *history_create(void) {
    int cap = config_get_hist_max();
    return history_create_with_cap(cap);
}

History *history_create_with_cap(int cap) {
    History *h = (History *) malloc(sizeof(History));
    if (h == NULL)
        return NULL;

    history_init(h, cap);

    if (h->cap <= 0 || h->items == NULL) {
        free(h);
        return NULL;
    }
    
    return h;
}

void history_destroy(History *h) {
    if (h == NULL)
        return;

    history_free(h);
    free(h);
}

/* Predicados e utilitarios: is_full, is_empty, size */
bool history_is_full(const History *h) {
    if (h == NULL || h->cap <= 0)
        return false;
        
    return (h->top >= (h->cap - 1));
}

bool history_is_empty(const History *h) {
    if (h == NULL || h->cap <= 0)
        return true;

    return (h->top < 0);
}

int history_size(const History *h) {
    if (h == NULL || h->cap <= 0)
        return 0;

    return (h->top + 1);
}

const char *history_top(const History *h) {
    if (h == NULL || h->top < 0 || h->cap <= 0)
        return NULL;
        
    return h->items + (size_t)h->top * (PROC_MAX_LEN + 1);
}

/* Inspecao para debug: imprime todas as entradas do historico */
void history_inspect(History *h) {
    if (h == NULL)
        return;

    util_printf("Historico (top = %d, cap = %d):\n", h->top, h->cap);

    for (int i = 0; i <= h->top; ++i) {
        util_printf("%d: %s\n", i, hist_at(h, i));
    }
}