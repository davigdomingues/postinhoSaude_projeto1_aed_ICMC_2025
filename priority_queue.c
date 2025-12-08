#include <stdlib.h>
#include <string.h>
#include "priority_queue.h"
#include "util.h"
#include "config.h"

/* Implementação de PriorityQueue:
 * - Heap binário com critério: menor prioridade primeiro; em empate, menor seq (mais antigo).
 * - Conjunto de membros (hash) para consulta O(1) de existência.
 * - Snapshot ordenado para acesso por índice sem alterar o heap.
 */

// Representa um item na fila de prioridade: id, prioridade e sequência de chegada (para desempate).
struct PQNode {
    char id[MAX_ID_LEN + 1];
    int priority;
    unsigned long seq;
};

// Entrada da tabela hash usada para membership O(1) por encadeamento separado.
struct QHashEntry {
    char *key;
    struct QHashEntry *next;
};

// Estrutura principal da fila de prioridade: heap binário + conjunto de membros para consulta rápida.
struct PriorityQueue {
    struct PQNode *nodes;
    int size;
    int cap;
    unsigned long seq_counter;
    struct QHashEntry **members;
    size_t mcap;
};

/* djb2: função de hash para strings usada pelo membership-set */
static unsigned long h_str(const char *s) {
    unsigned long h = 5381;
    int c;

    while ((c = (unsigned char)*s++))
        h = ((h << 5) + h) + c;

    return h;
}

/* Inicializa tabela hash de membros (buckets alocados dinamicamente) */
static int h_init(struct PriorityQueue *pq, size_t buckets) {
    pq->members = (struct QHashEntry**)calloc(buckets, sizeof(struct QHashEntry*));

    if (!pq->members)
        return -1;

    pq->mcap = buckets;
    return 0;
}

/* Adiciona um id ao conjunto de membros, evitando duplicatas */
static void h_add(struct PriorityQueue *pq, const char *id) {
    if (!pq || !pq->members || !id) return;

    unsigned long b = h_str(id) % pq->mcap;
    struct QHashEntry *e = pq->members[b];

    while (e) {
        if (strcmp(e->key, id) == 0) return;
        e = e->next;
    }

    e = (struct QHashEntry*)malloc(sizeof(*e));

    if (!e) 
        return;

    e->key = util_strdup(id);

    if (!e->key) {
        free(e);
        return;
    }

    e->next = pq->members[b];
    pq->members[b] = e;
}

/* Remove um id do conjunto de membros */
static void h_remove(struct PriorityQueue *pq, const char *id) {
    if (!pq || !pq->members || !id) return;

    unsigned long b = h_str(id) % pq->mcap;
    struct QHashEntry **pe = &pq->members[b];

    while (*pe) {
        if (strcmp((*pe)->key, id) == 0) {
            struct QHashEntry *r = *pe;
            *pe = r->next;

            free(r->key);
            free(r);

            return;
        }

        pe = &((*pe)->next);
    }
}

/* Verifica se um id está presente no conjunto de membros (1/0) */
static int h_contains(const struct PriorityQueue *pq, const char *id) {
    if (!pq || !pq->members || !id) return 0;

    unsigned long b = h_str(id) % pq->mcap;
    struct QHashEntry *e = pq->members[b];

    while (e) {

        if (strcmp(e->key, id) == 0) 
            return 1;

        e = e->next;
    }

    return 0;
}

/* Libera toda a memória da tabela hash de membros */
static void h_free(struct PriorityQueue *pq) {
    if (!pq || !pq->members) return;

    for (size_t i = 0; i < pq->mcap; ++i) {
        struct QHashEntry *e = pq->members[i];

        while (e) {
            struct QHashEntry *n = e->next;

            free(e->key);
            free(e);

            e = n;
        }
    }

    free(pq->members);
    
    pq->members = NULL;
    pq->mcap = 0;
}

/* Critério de ordenação do heap: menor prioridade primeiro; em empate, menor seq (mais antigo) */
static int better(const struct PQNode *a, const struct PQNode *b) {
    if (a->priority != b->priority)
        return a->priority < b->priority;

    return a->seq < b->seq;
}

/* Troca dois nós do heap (utilitário) */
static void swap_nodes(struct PQNode *a, struct PQNode *b) {
    struct PQNode tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Sobe um nó no heap até restaurar a propriedade de heap (heapify-up) */
static void heap_up(struct PriorityQueue *pq, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;

        if (better(&pq->nodes[idx], &pq->nodes[parent])) {
            swap_nodes(&pq->nodes[idx], &pq->nodes[parent]);
            idx = parent;
        } 
        
        else 
            break;
    }
}

/* Desce um nó no heap até restaurar a propriedade de heap (heapify-down) */
static void heap_down(struct PriorityQueue *pq, int idx) {
    for (;;) {
        int l = idx * 2 + 1;
        int r = idx * 2 + 2;
        int best = idx;

        if (l < pq->size && better(&pq->nodes[l], &pq->nodes[best]))
            best = l;
            
        if (r < pq->size && better(&pq->nodes[r], &pq->nodes[best]))
            best = r;

        if (best != idx) {
            swap_nodes(&pq->nodes[idx], &pq->nodes[best]);
            idx = best;
        } else break;
    }
}

/* Cria a fila de prioridade com capacidade fixa; inicializa heap e conjunto de membros */
PriorityQueue *pqueue_create(int cap) {
    /* Permite capacidade dinâmica; padrão para capacidade de espera configurada em tempo de execução se inválido */
    int max_cap = config_get_wait_cap();

    if (cap <= 0) 
        cap = max_cap;

    if (cap > max_cap) 
        return NULL;

    PriorityQueue *pq = (PriorityQueue*)malloc(sizeof(*pq));
    if (!pq) return NULL;

    pq->nodes = (struct PQNode*)malloc(sizeof(struct PQNode) * cap);
    if (!pq->nodes) {
        free(pq);
        return NULL;
    }

    pq->size = 0;
    pq->cap = cap;
    pq->seq_counter = 0;
    pq->members = NULL;
    pq->mcap = 0;

    if (h_init(pq, (size_t)cap * 2 + 3) != 0) {
        free(pq->nodes);
        free(pq);
        return NULL;
    }

    return pq;
}

/* Destroi a fila: libera heap e tabela de membros */
void pqueue_destroy(PriorityQueue *pq) {
    if (!pq) return;
    h_free(pq);
    free(pq->nodes);
    free(pq);
}

/* Predicado: retorna 1 se a fila está cheia, 0 caso contrário */
int pqueue_is_full(const PriorityQueue *pq) {
    return (pq && pq->size >= pq->cap) ? 1 : 0;
}

/* Consulta de membership: 1 se contém, 0 caso contrário (via hash) */
int pqueue_contains(const PriorityQueue *pq, const char *id) {
    return h_contains(pq, id);
}

/* Retorna o número de elementos atualmente na fila */
int pqueue_size(const PriorityQueue *pq) {
    return pq ? pq->size : 0;
}

/* Enfileira um id com prioridade: valida entradas, insere no heap e atualiza conjunto de membros */
int pqueue_enqueue(PriorityQueue *pq, const char *id, int priority) {
    if (!pq || !id) 
        return -1;

    if (priority < 1 || priority > 5) 
        return -1;
    
    if (pqueue_is_full(pq)) 
        return -1;
    
    if (h_contains(pq, id)) 
        return -1;

    int idx = pq->size++;
    strncpy(pq->nodes[idx].id, id, MAX_ID_LEN);
    pq->nodes[idx].id[MAX_ID_LEN] = '\0';
    pq->nodes[idx].priority = priority;
    pq->nodes[idx].seq = pq->seq_counter++;

    heap_up(pq, idx);
    h_add(pq, id);

    return 0;
}

/* Remove o elemento de maior prioridade (menor valor) e copia seu id para out */
int pqueue_dequeue(PriorityQueue *pq, char *out, size_t out_size) {
    if (!pq || !out || out_size == 0) 
        return -1;
    
    if (pq->size == 0) 
        return -1;

    strncpy(out, pq->nodes[0].id, out_size - 1);
    out[out_size - 1] = '\0';

    h_remove(pq, out);

    pq->nodes[0] = pq->nodes[pq->size - 1];
    pq->size--;

    heap_down(pq, 0);

    return 0;
}

/* Snapshot ordenado (n pequeno) */
static int build_sorted(const PriorityQueue *pq, struct PQNode *buf, int n) {
    for (int i = 0; i < n; ++i)
        buf[i] = pq->nodes[i];

    /* insertion sort */
    for (int i = 1; i < n; ++i) {
        struct PQNode key = buf[i];
        int j = i - 1;

        while (j >= 0 && !better(&buf[j], &key)) {
            buf[j + 1] = buf[j];
            --j;
        }

        buf[j + 1] = key;
    }

    return 0;
}

/* Obtém o id pelo índice lógico na ordenação por prioridade (não altera o heap) */
int pqueue_get_id_by_index(const PriorityQueue *pq, int index, char *out, size_t out_size) {
    if (!pq || !out || out_size == 0) 
        return -1;

    if (index < 0 || index >= pq->size) 
        return -1;

    /* Aloca buffer snapshot dimensionado para o tamanho atual da fila */
    int n = pq->size;
    struct PQNode *tmp = (struct PQNode *)malloc(sizeof(struct PQNode) * (size_t)n);
    
    if (!tmp) 
        return -1;

    build_sorted(pq, tmp, n);
    strncpy(out, tmp[index].id, out_size - 1);
    out[out_size - 1] = '\0';

    free(tmp);
    return 0;
}

/* Obtém a prioridade pelo índice lógico na ordenação por prioridade (não altera o heap) */
int pqueue_get_priority_by_index(const PriorityQueue *pq, int index, int *out_priority) {
    if (!pq || !out_priority) 
        return -1;

    if (index < 0 || index >= pq->size) 
        return -1;

    int n = pq->size;
    struct PQNode *tmp = (struct PQNode *)malloc(sizeof(struct PQNode) * (size_t)n);
    
    if (!tmp) 
        return -1;

    build_sorted(pq, tmp, n);
    *out_priority = tmp[index].priority;
    free(tmp);
    return 0;
}