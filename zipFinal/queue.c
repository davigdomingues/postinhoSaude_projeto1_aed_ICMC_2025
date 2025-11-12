#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "queue.h"
#include "util.h"

/* Definição interna de Queue (TAD opaco no header)
   (comentário existente mantido)

   Observação de persistência:
   - A representação interna é serializada/deserializada por io.c; este ficheiro não realiza I/O em disco.
   - Para acelerar membership (queue_contains) a implementação usa um conjunto baseado em hash
     (encadeamento separado), transparente para os utilizadores do TAD.
*/

struct Queue {
    char ids[WAIT_CAP][MAX_ID_LEN + 1];
    int head; /* índice do primeiro elemento */
    int size; /* número de elementos */
    int cap;  /* capacidade efetiva (<= WAIT_CAP) */
    /* hash simples para verificação de membros (encadeamento separado) */
    struct QHashEntry **members;
    size_t mcap;
};

/* Entrada da tabela hash usada como conjunto de membros (membership set) */
struct QHashEntry {
    char *key;
    struct QHashEntry *next;
};

static unsigned long q_hash_str(const char *s) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*s++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

static int qset_init(Queue *q, size_t buckets) {
    q->members = (struct QHashEntry **)calloc(buckets, sizeof(struct QHashEntry *));
    if (!q->members) return -1;
    q->mcap = buckets;
    return 0;
}

/* liberta todos os elementos do conjunto de membros (hash-set) */
static void qset_free(Queue *q) {
    if (!q || !q->members) return;
    for (size_t i = 0; i < q->mcap; ++i) {
        struct QHashEntry *e = q->members[i];
        while (e) {
            struct QHashEntry *n = e->next;
            free(e->key);
            free(e);
            e = n;
        }
    }
    free(q->members);
    q->members = NULL;
    q->mcap = 0;
}

/* adiciona um id ao conjunto de membros (se nao existir) */
static void qset_add(Queue *q, const char *id) {
    if (!q || !q->members || !id) 
        return;

    unsigned long h = q_hash_str(id) % q->mcap;
    struct QHashEntry *e = q->members[h];

    while (e) { 
        if (strcmp(e->key, id) == 0) 
        return;

        e = e->next;
   
    }
    e = malloc(sizeof(*e));
    if (!e) 
        return;

    e->key = strdup(id);
    if (!e->key) { 
        free(e); 
        return; 
    }
    
    e->next = q->members[h];
    q->members[h] = e;
}

/* remove um id do conjunto de membros */
static void qset_remove(Queue *q, const char *id) {
    if (!q || !q->members || !id) return;
    unsigned long h = q_hash_str(id) % q->mcap;
    struct QHashEntry **pe = &q->members[h];
    while (*pe) {
        if (strcmp((*pe)->key, id) == 0) {
            struct QHashEntry *rem = *pe;
            *pe = rem->next;
            free(rem->key);
            free(rem);
            return;
        }
        pe = &((*pe)->next);
    }
}

/* verifica rapidamente se o id pertence ao conjunto (1) ou nao (0) */
static int qset_contains(const Queue *q, const char *id) {
    if (!q || !q->members || !id) return 0;
    unsigned long h = q_hash_str(id) % q->mcap;
    struct QHashEntry *e = q->members[h];
    while (e) { if (strcmp(e->key, id) == 0) return 1; e = e->next; }
    return 0;
}

/* Inicializa estrutura interna da fila (head=0, size=0, cap=cap) */
static int queue_init(Queue *q, int cap) {
    if (!q || cap <= 0 || cap > WAIT_CAP) return -1;
    q->head = 0;
    q->size = 0;
    q->cap = cap;
    return 0;
}

/* Enfileira no buffer circular:
   - calcula indice de insercao = (head + size) % cap
   - copia id com strncpy e garante terminação
   - incrementa size
   - retorna -1 se fila cheia ou parametros invalidos */
int queue_enqueue(Queue *q, const char *id) {
    if (!q || !id) return -1;
    if (q->size >= q->cap) return -1;
    int idx = (q->head + q->size) % q->cap;
    strncpy(q->ids[idx], id, MAX_ID_LEN);
    q->ids[idx][MAX_ID_LEN] = '\0';
    q->size++;
    /* atualizar conjunto de membros */
    if (q->members) qset_add(q, id);
    return 0;
}

/* Desenfileira do inicio:
   - copia id do head para 'out' de forma segura (out_size)
   - atualiza head = (head + 1) % cap e decrementa size
   - retorna -1 se fila vazia
*/
int queue_dequeue(Queue *q, char *out, size_t out_size) {
    if (!q || !out || out_size == 0) return -1;
    if (q->size == 0) return -1;
    strncpy(out, q->ids[q->head], out_size - 1);
    out[out_size - 1] = '\0';
    /* remove do conjunto de membros */
    if (q->members) qset_remove(q, out);
    q->head = (q->head + 1) % q->cap;
    q->size--;
    return 0;
}

/* Verifica se a fila esta cheia (1) ou nao (0) */
int queue_is_full(const Queue *q) {
    return (q && q->size >= q->cap) ? 1 : 0;
}

/* Verifica existencia de um ID na fila (1 presente, 0 ausente) */
int queue_contains(const Queue *q, const char *id) {
    /* usa hash-set membership se disponível */
    if (!q || !id) return 0;
    if (q->members) return qset_contains(q, id);
    /* fallback */
    for (int i = 0, idx = q->head; i < q->size; ++i, idx = (idx + 1) % q->cap)
        if (strcmp(q->ids[idx], id) == 0) return 1;
    return 0;
}

/* Imprime a fila em ordem logica:
   - itera a partir de head e avanca modulo cap para respeitar wrap-around
   - imprime posicao (1-based) e id correspondente
*/
void queue_print(const Queue *q) {
    if (!q) return;
    
    for (int i = 0, idx = q->head; i < q->size; ++i, idx = (idx + 1) % q->cap)
        util_printf("%d: %s\n", i + 1, q->ids[idx]);
}

/* Criacao / destruicao da fila (aloca o TAD opaco) */
Queue* queue_create(int cap) {
    Queue *q = malloc(sizeof(Queue));
    if (!q) return NULL;
    if (queue_init(q, cap) != 0) { free(q); return NULL; }
    /* inicializa conjunto de membros com um pequeno número de buckets */
    if (qset_init(q, (size_t)cap * 2 + 3) != 0) { free(q); return NULL; }
    return q;
}

void queue_destroy(Queue *q) {
    if (!q) return;
    qset_free(q);
    free(q);
}

/* Consultas auxiliares: tamanho e obter ID por indice logico na fila */
int queue_size(const Queue *q) {
    return q ? q->size : 0;
}

int queue_get_id_by_index(const Queue *q, int index, char *out, size_t out_size) {
    if (!q || !out || out_size == 0) return -1;
    if (index < 0 || index >= q->size) return -1;
    int idx = (q->head + index) % q->cap;
    strncpy(out, q->ids[idx], out_size - 1);
    out[out_size - 1] = '\0';
    return 0;
}