#include <stdlib.h>
#include <string.h>
#include "priority_queue.h"
#include "util.h"

struct PQNode {
    char id[MAX_ID_LEN + 1];
    int priority;
    unsigned long seq;
};

struct QHashEntry {
    char *key;
    struct QHashEntry *next;
};

struct PriorityQueue {
    struct PQNode *nodes;
    int size;
    int cap;
    unsigned long seq_counter;
    struct QHashEntry **members;
    size_t mcap;
};

/* djb2 */
static unsigned long h_str(const char *s) {
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*s++))
        h = ((h << 5) + h) + c;
    return h;
}

static int h_init(struct PriorityQueue *pq, size_t buckets) {
    pq->members = (struct QHashEntry**)calloc(buckets, sizeof(struct QHashEntry*));
    if (!pq->members) return -1;
    pq->mcap = buckets;
    return 0;
}

static void h_add(struct PriorityQueue *pq, const char *id) {
    if (!pq || !pq->members || !id) return;

    unsigned long b = h_str(id) % pq->mcap;
    struct QHashEntry *e = pq->members[b];

    while (e) {
        if (strcmp(e->key, id) == 0) return;
        e = e->next;
    }

    e = (struct QHashEntry*)malloc(sizeof(*e));
    if (!e) return;

    e->key = util_strdup(id);
    if (!e->key) {
        free(e);
        return;
    }

    e->next = pq->members[b];
    pq->members[b] = e;
}

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

static int h_contains(const struct PriorityQueue *pq, const char *id) {
    if (!pq || !pq->members || !id) return 0;

    unsigned long b = h_str(id) % pq->mcap;
    struct QHashEntry *e = pq->members[b];

    while (e) {
        if (strcmp(e->key, id) == 0) return 1;
        e = e->next;
    }
    return 0;
}

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

/* heap ordering: menor priority primeiro; desempate menor seq */
static int better(const struct PQNode *a, const struct PQNode *b) {
    if (a->priority != b->priority)
        return a->priority < b->priority;
    return a->seq < b->seq;
}

static void swap_nodes(struct PQNode *a, struct PQNode *b) {
    struct PQNode tmp = *a;
    *a = *b;
    *b = tmp;
}

static void heap_up(struct PriorityQueue *pq, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (better(&pq->nodes[idx], &pq->nodes[parent])) {
            swap_nodes(&pq->nodes[idx], &pq->nodes[parent]);
            idx = parent;
        } else break;
    }
}

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

PriorityQueue *pqueue_create(int cap) {
    if (cap <= 0 || cap > WAIT_CAP) return NULL;

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

void pqueue_destroy(PriorityQueue *pq) {
    if (!pq) return;
    h_free(pq);
    free(pq->nodes);
    free(pq);
}

int pqueue_is_full(const PriorityQueue *pq) {
    return (pq && pq->size >= pq->cap) ? 1 : 0;
}

int pqueue_contains(const PriorityQueue *pq, const char *id) {
    return h_contains(pq, id);
}

int pqueue_size(const PriorityQueue *pq) {
    return pq ? pq->size : 0;
}

int pqueue_enqueue(PriorityQueue *pq, const char *id, int priority) {
    if (!pq || !id) return -1;
    if (priority < 1 || priority > 5) return -1;
    if (pqueue_is_full(pq)) return -1;
    if (h_contains(pq, id)) return -1;

    int idx = pq->size++;
    strncpy(pq->nodes[idx].id, id, MAX_ID_LEN);
    pq->nodes[idx].id[MAX_ID_LEN] = '\0';
    pq->nodes[idx].priority = priority;
    pq->nodes[idx].seq = pq->seq_counter++;

    heap_up(pq, idx);
    h_add(pq, id);

    return 0;
}

int pqueue_dequeue(PriorityQueue *pq, char *out, size_t out_size) {
    if (!pq || !out || out_size == 0) return -1;
    if (pq->size == 0) return -1;

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

int pqueue_get_id_by_index(const PriorityQueue *pq, int index, char *out, size_t out_size) {
    if (!pq || !out || out_size == 0) return -1;
    if (index < 0 || index >= pq->size) return -1;

    struct PQNode tmp[WAIT_CAP];
    build_sorted(pq, tmp, pq->size);

    strncpy(out, tmp[index].id, out_size - 1);
    out[out_size - 1] = '\0';

    return 0;
}

int pqueue_get_priority_by_index(const PriorityQueue *pq, int index, int *out_priority) {
    if (!pq || !out_priority) return -1;
    if (index < 0 || index >= pq->size) return -1;

    struct PQNode tmp[WAIT_CAP];
    build_sorted(pq, tmp, pq->size);

    *out_priority = tmp[index].priority;
    return 0;
}