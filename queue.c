#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "queue.h"

/* Definição interna de Queue (TAD opaco no header)
   (comentário existente mantido)

   Observação de persistência:
   - A representação interna é serializada/deserializada por io.c; este ficheiro não realiza I/O em disco.
*/

struct Queue {
    char ids[WAIT_CAP][MAX_ID_LEN + 1];
    int head; /* índice do primeiro elemento */
    int size; /* número de elementos */
    int cap;  /* capacidade efetiva (<= WAIT_CAP) */
};

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
    q->head = (q->head + 1) % q->cap;
    q->size--;
    return 0;
}

/* Libera recursos internos (nenhum atualmente) */
static void queue_free(Queue *q) {
    (void)q;
}

/* Verifica se a fila esta cheia (1) ou nao (0) */
int queue_is_full(const Queue *q) {
    return (q && q->size >= q->cap) ? 1 : 0;
}

/* Verifica existencia de um ID na fila (1 presente, 0 ausente) */
int queue_contains(const Queue *q, const char *id) {
    if (!q || !id) return 0;
    for (int i = 0, idx = q->head; i < q->size; ++i, idx = (idx + 1) % q->cap) {
        if (strcmp(q->ids[idx], id) == 0) return 1;
    }
    return 0;
}

/* Imprime a fila em ordem logica:
   - itera a partir de head e avanca modulo cap para respeitar wrap-around
   - imprime posicao (1-based) e id correspondente
*/
void queue_print(const Queue *q) {
    if (!q) return;
    
    for (int i = 0, idx = q->head; i < q->size; ++i, idx = (idx + 1) % q->cap)
        printf("%d: %s\n", i + 1, q->ids[idx]);
}

/* Remove um id arbitrario:
   - procura o id iterando a partir de head (wrap-around)
   - ao encontrar desloca os elementos subsequentes uma posicao para a esquerda,
     preservando a ordem logica da fila (operacao O(n))
   - decrementa size
*/
int queue_remove(Queue *q, const char *id) {
    if (!q || !id) return -1;
    for (int i = 0, idx = q->head; i < q->size; ++i, idx = (idx + 1) % q->cap) {
        if (strcmp(q->ids[idx], id) == 0) {
            /* desloca elementos para a esquerda mantendo circularidade */
            for (int j = i; j < q->size - 1; ++j) {
                int from = (q->head + j + 1) % q->cap;
                int to = (q->head + j) % q->cap;
                strncpy(q->ids[to], q->ids[from], MAX_ID_LEN + 1);
            }
            q->size--;
            return 0;
        }
    }
    return -1;
}

/* Criacao / destruicao da fila (aloca o TAD opaco) */
Queue* queue_create(int cap) {
    Queue *q = malloc(sizeof(Queue));
    if (!q) return NULL;
    if (queue_init(q, cap) != 0) { free(q); return NULL; }
    return q;
}

void queue_destroy(Queue *q) {
    if (!q) return;
    queue_free(q);
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