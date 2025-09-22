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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "patient_list.h"

static int plist_grow(PatientList *pl) {
    size_t ncap = pl->cap ? pl->cap * 2 : 16;
    Patient *nd = realloc(pl->data, ncap * sizeof(Patient));
    
    if (!nd) return -1;

    pl->data = nd; pl->cap = ncap; return 0;
}

void plist_init(PatientList *pl) { 
    pl->data = NULL; pl->size = 0; pl->cap = 0;
}

void plist_free(PatientList *pl) { 
    if (!pl) return;
    if (pl->data) {
        for (size_t i = 0; i < pl->size; ++i) {
            history_free(&pl->data[i].hist);
        }
        free(pl->data);
    }
    pl->data = NULL; pl->size = 0; pl->cap = 0;
}

void plist_clear(PatientList *pl) {
    if (!pl) return;
    if (pl->data) {
        for (size_t i = 0; i < pl->size; ++i) history_free(&pl->data[i].hist);
    }
    pl->size = 0;
}

int plist_reserve(PatientList *pl, size_t new_cap) {
    if (!pl) return -1;
    if (new_cap <= pl->cap) return 0;
    Patient *nd = realloc(pl->data, new_cap * sizeof(Patient));
    if (!nd) return -1;
    pl->data = nd; pl->cap = new_cap; return 0;
}

int plist_shrink_to_fit(PatientList *pl) {
    if (!pl) return -1;
    if (pl->size == 0) {
        free(pl->data); pl->data = NULL; pl->cap = 0; return 0;
    }
    if (pl->size == pl->cap) return 0;
    Patient *nd = realloc(pl->data, pl->size * sizeof(Patient));
    if (!nd) return -1;
    pl->data = nd; pl->cap = pl->size; return 0;
}

int plist_find_index(const PatientList *pl, const char *id) {
    if (!pl || !id) return -1;
    for (size_t i = 0; i < pl->size; ++i) {
        if (strcmp(pl->data[i].id, id) == 0)
            return (int)i;
    }
    return -1;
}

Patient *plist_get(PatientList *pl, const char *id) {
    int idx = plist_find_index(pl, id); return idx < 0 ? NULL : &pl->data[idx];
}

Patient *plist_get_by_index(PatientList *pl, size_t idx) {
    if (!pl || idx >= pl->size) return NULL;
    return &pl->data[idx];
}

int plist_insert(PatientList *pl, const char *id, const char *name) {
    if (!pl || !id || !*id || !name || !*name) return -2;
    if (plist_find_index(pl, id) >= 0) return -1;
    if (pl->size == pl->cap && plist_grow(pl) < 0) return -2;
    Patient *p = &pl->data[pl->size++];
    strncpy(p->id, id, MAX_ID_LEN); p->id[MAX_ID_LEN] = '\0';
    strncpy(p->name, name, MAX_NAME_LEN); p->name[MAX_NAME_LEN] = '\0';
    history_init(&p->hist);
    p->called = false;
    return 0;
}

int plist_remove(PatientList *pl, const char *id) {
    if (!pl || !id) return -1;
    int idx = plist_find_index(pl, id); if (idx < 0) return -1;
    history_free(&pl->data[idx].hist);
    pl->data[idx] = pl->data[pl->size - 1]; pl->size--; return 0;
}

int plist_update_name(PatientList *pl, const char *id, const char *new_name) {
    if (!pl || !id || !new_name) return -1;
    Patient *p = plist_get(pl, id); if (!p) return -1;
    strncpy(p->name, new_name, MAX_NAME_LEN); p->name[MAX_NAME_LEN] = '\0'; return 0;
}

int plist_set_called(PatientList *pl, const char *id, bool called) {
    if (!pl || !id) return -1;
    Patient *p = plist_get(pl, id); if (!p) return -1;
    p->called = called; return 0;
}

void plist_print(const PatientList *pl) {
    if (!pl) return;
    printf("Pacientes registrados: %zu\n", pl->size);
    if (pl->size == 0) { printf("Nenhum paciente registrado.\n"); return; }
    for (size_t i = 0; i < pl->size; ++i) {
        printf("- ID: %s | Nome: %s | Procedimentos: %d | Chamado: %s\n",
               pl->data[i].id, pl->data[i].name, pl->data[i].hist.top + 1,
               pl->data[i].called ? "SIM" : "NAO");
    }
}