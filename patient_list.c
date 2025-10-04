/* Explicação:
 *
 * Propósito:
 * - Implementar uma lista dinâmica de pacientes (PatientList) com operações básicas:
 *   inicializar, liberar, buscar por ID, obter ponteiro para paciente, inserir,
 *   remover e imprimir lista.
 *
 * Implementação e detalhes:
 *  - O array interno cresce com plist_grow(): nova capacidade = cap ? cap*2 : 16.
 *   Usa realloc e atualiza pl->data/pl->cap; falha de alocação retorna erro.
 * 
 * - Ao inserir, strncpy é usado para copiar id e name e o terminador '\0' é
 *   forçado explicitamente (protege contra truncamento sem terminador).
 * 
 * - Ao remover, a técnica de mover o último item para a posição removida torna
 *   a operação constante em tempo, mas altera a ordem dos elementos.
 *
 * Dependências e pressupostos:
 * - Usa Patient, History e constantes de config (MAX_ID_LEN, MAX_NAME_LEN).
 * - Pressupõe existência de history_init(&p->hist) para inicializar o histórico.
 * - A impressão assume que History possui campo `top` com o índice do topo (top >= -1).
 *
 * Observações de integração:
 * - PatientList delega persistência em disco para io.c; funções aqui são apenas manipulação em memória.
 * - Ao remover um paciente chamamos history_free() para limpar o histórico antes de sobrescrever/soltar memória.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "patient_list.h"
#include "config.h"
#include "history.h"

/* Definição interna de History (visível apenas neste translation unit).
   Mantém o tipo History opaco publicamente (history.h) mas permite que
   patient_list.c declare Patient com um campo History diretamente. */

/* Definições internas */
struct Patient {
    char id[MAX_ID_LEN + 1];
    char name[MAX_NAME_LEN + 1];
    History *hist; /* ponteiro para history opaco: preserva encapsulamento */
    bool called; /* true se o paciente já foi chamado para atendimento */
};

/* PatientList pode ser alocado externamente via plist_create; definimos aqui o layout */
struct PatientList {
    Patient *data;
    size_t size, cap;
};

/* Helpers estáticos */
static int plist_grow(PatientList *pl) {
    size_t ncap = pl->cap ? pl->cap * 2 : 16;
    Patient *nd = realloc(pl->data, ncap * sizeof(Patient));
    if (!nd) return -1;
    pl->data = nd;
    pl->cap = ncap;
    return 0;
}

static void plist_init(PatientList *pl) {
    pl->data = NULL;
    pl->size = 0;
    pl->cap = 0;
}

static void plist_free(PatientList *pl) {
    if (!pl) 
        return;

    if (pl->data) {
        for (size_t i = 0; i < pl->size; ++i) {
            if (pl->data[i].hist) {
                history_destroy(pl->data[i].hist);
                pl->data[i].hist = NULL;
            }
        }
        free(pl->data);
    }

    pl->data = NULL;
    pl->size = 0;
    pl->cap = 0;
}

static Patient *plist_get(PatientList *pl, const char *id) {
    int idx = plist_find_index(pl, id);
    return idx < 0 ? NULL : &pl->data[idx];
}

int plist_find_index(const PatientList *pl, const char *id) {
    if (!pl || !id) 
        return -1;

    for (size_t i = 0; i < pl->size; ++i) {
        if (strcmp(pl->data[i].id, id) == 0)
            return (int)i;
    }

    return -1;
}

int plist_insert(PatientList *pl, const char *id, const char *name) {
    if (!pl || !id || !*id || !name || !*name) 
        return -2;
    
    if (plist_find_index(pl, id) >= 0) 
        return -1;

    if (pl->size == pl->cap && plist_grow(pl) < 0) 
        return -2;

    Patient *p = &pl->data[pl->size++];
    strncpy(p->id, id, MAX_ID_LEN); p->id[MAX_ID_LEN] = '\0';
    strncpy(p->name, name, MAX_NAME_LEN); p->name[MAX_NAME_LEN] = '\0';
    /* criar History via API centralizada */
    p->hist = history_create();
    if (!p->hist) {
        /* falha de alocação: desfaz incremento de size */
        pl->size--;
        return -2;
    }
    p->called = false;

    return 0;
}

int plist_remove(PatientList *pl, const char *id) {
    if (!pl || !id) 
        return -1;

    int idx = plist_find_index(pl, id);
    
    if (idx < 0) {
        return -1;
    }

    /* remover histórico do paciente a ser excluído e substituir pelo último elemento */
    if (pl->data[idx].hist) {
        history_destroy(pl->data[idx].hist);
        pl->data[idx].hist = NULL;
    }
    pl->data[idx] = pl->data[pl->size - 1];
    pl->size--;
    
    return 0;
}

int plist_set_called(PatientList *pl, const char *id, bool called) {
    if (!pl || !id) 
        return -1;

    Patient *p = plist_get(pl, id);
    if (!p) 
        return -1;
    
    p->called = called;
    return 0;
}

/* Impressão e utilitários públicos menores */
void plist_print(const PatientList *pl) {
    if (!pl) 
        return;

    /* imprimir size_t de forma portável usando unsigned long cast */
    printf("Pacientes registrados: %lu\n", (unsigned long)pl->size);

    if (pl->size == 0) { 
        printf("Nenhum paciente registrado.\n"); 
        return; 
    }
    
    for (size_t i = 0; i < pl->size; ++i) {
        int hcount = pl->data[i].hist ? history_size(pl->data[i].hist) : 0;
        printf("- ID: %s | Nome: %s | Procedimentos: %d | Chamado: %s\n",
               pl->data[i].id, pl->data[i].name, hcount,
               pl->data[i].called ? "SIM" : "NAO");
    }
}

/* Criação / destruição do TAD opaco */
PatientList* plist_create(void) {
    PatientList *pl = malloc(sizeof(PatientList));
    if (!pl) 
        return NULL;

    plist_init(pl);
    return pl;
}

void plist_destroy(PatientList *pl) {
    if (!pl) 
        return;

    plist_free(pl);
    free(pl);
}

size_t plist_size(const PatientList *pl) {
    return pl ? pl->size : 0;
}

int plist_get_id_by_index(const PatientList *pl, size_t idx, char *out, size_t out_size) {
    if (!pl || !out || out_size == 0) 
        return -1;

    if (idx >= pl->size)
        return -1;

    strncpy(out, pl->data[idx].id, out_size - 1);
    out[out_size - 1] = '\0';

    return 0;
}

int plist_get_name_by_index(const PatientList *pl, size_t idx, char *out, size_t out_size) {
    if (!pl || !out || out_size == 0)
        return -1;

    if (idx >= pl->size)
        return -1;
        
    strncpy(out, pl->data[idx].name, out_size - 1);
    out[out_size - 1] = '\0';
    return 0;
}

int plist_get_name_by_id(const PatientList *pl, const char *id, char *out, size_t out_size) {
    if (!pl || !id || !out || out_size == 0)
        return -1;

    int idx = plist_find_index(pl, id);
    if (idx < 0)
        return -1;

    return plist_get_name_by_index(pl, (size_t)idx, out, out_size);
}

int plist_is_called(const PatientList *pl, const char *id) {
    if (!pl || !id) 
        return 0;

    int idx = plist_find_index(pl, id);
    if (idx < 0) 
        return 0;

    return pl->data[idx].called ? 1 : 0;
}

/* Histórico: wrappers que usam history_* internamente */
int plist_history_is_full(const PatientList *pl, const char *id) {
    if (!pl || !id) 
        return 0;

    int idx = plist_find_index(pl, id);

    return (idx < 0 || !pl->data[idx].hist) ? 0 : (history_is_full(pl->data[idx].hist) ? 1 : 0);
}

int plist_history_push(PatientList *pl, const char *id, const char *proc) {
    if (!pl || !id || !proc) 
        return -1;

    int idx = plist_find_index(pl, id);

    return (idx < 0 || !pl->data[idx].hist) ? -1 : history_push(pl->data[idx].hist, proc);
}

int plist_history_pop(PatientList *pl, const char *id, char *out, size_t out_size) {
    if (!pl || !id || !out || out_size == 0) 
        return -1;

    int idx = plist_find_index(pl, id);

    return (idx < 0 || !pl->data[idx].hist) ? -1 : history_pop(pl->data[idx].hist, out, out_size);
}

int plist_history_size_by_id(const PatientList *pl, const char *id) {
    if (!pl || !id) 
        return 0;

    int idx = plist_find_index(pl, id);

    return (idx < 0 || !pl->data[idx].hist) ? 0 : history_size(pl->data[idx].hist);
}

int plist_history_get_by_id(const PatientList *pl, const char *id, int hist_idx, char *out, size_t out_size) {
    if (!pl || !id || !out || out_size == 0) 
        return -1;

    int idx = plist_find_index(pl, id);

    if (idx < 0) 
        return -1;

    if (!pl->data[idx].hist) return -1;
    return history_get_by_index(pl->data[idx].hist, hist_idx, out, out_size);
}

/* por índice (útil para serialização em io.c) */
int plist_history_size_by_index(const PatientList *pl, size_t patient_idx) {
    if (!pl || patient_idx >= pl->size) 
        return 0;

    return pl->data[patient_idx].hist ? history_size(pl->data[patient_idx].hist) : 0;
}

/* Novamente: obter entrada do histórico por índice do paciente.
   Retorna 0 em sucesso, -1 em erro (índices inválidos / ponteiros nulos). */
int plist_history_get_by_index(const PatientList *pl, size_t patient_idx, int hist_idx, char *out, size_t out_size) {
    if (!pl || !out || out_size == 0) 
        return -1;

    if (patient_idx >= pl->size) 
        return -1;

    if (!pl->data[patient_idx].hist) return -1;
    return history_get_by_index(pl->data[patient_idx].hist, hist_idx, out, out_size);
    
    return 0;
}