/* Explicação:
 *
 * Funções principais:
 * - plist_init(PatientList *pl)
 *     Inicializa a estrutura definindo data = NULL, size = 0, cap = 0.
 *
 * - plist_free(PatientList *pl)
 *     Liberta a memória do array interno pl->data e zera campos; não libera
 *     memórias internas de History (presume que History não aloca ou possui
 *     função própria para liberar, caso necessário deve ser chamada aqui).
 *
 * - plist_find_index(const PatientList *pl, const char *id)
 *     Procura linearmente pelo paciente com o ID fornecido; retorna índice
 *     (int) se encontrado ou -1 se não encontrado / id inválido.
 *
 * - Patient* plist_get(PatientList *pl, const char *id)
 *     Retorna ponteiro para o paciente com o ID (ou NULL se não encontrar).
 *
 * - int plist_insert(PatientList *pl, const char *id, const char *name)
 *     Insere um novo paciente no final da lista (apêndice). Valida id e name,
 *     evita duplicatas e expande a capacidade do array se necessário.
 *     Códigos de retorno:
 *       0 -> sucesso
 *      -1 -> ID duplicado
 *      -2 -> entrada inválida (id/name vazios) ou erro de memória (realloc)
 *
 * - int plist_remove(PatientList *pl, const char *id)
 *     Remove o paciente com o ID especificado, substituindo-o pelo último
 *     elemento do array (remoção O(1), não preserva ordem). Retorna 0 sucesso,
 *     -1 se não encontrar.
 *
 * - void plist_print(const PatientList *pl)
 *     Imprime uma lista simples de pacientes com ID, nome e número de procedimentos
 *     (usando p->hist.top + 1 para contar entradas do histórico).
 */

#ifndef PATIENT_LIST_H
#define PATIENT_LIST_H

#include <stddef.h>
#include <stdbool.h>
#include "config.h"
#include "history.h" /* Necessário para o campo 'hist' */

typedef struct {
    char id[MAX_ID_LEN + 1];
    char name[MAX_NAME_LEN + 1];
    History hist;
    bool called; /* true se o paciente já foi chamado para atendimento */
} Patient;

typedef struct {
    Patient *data;
    size_t size, cap;
} PatientList;

/* Inicialização / finalização */
void  plist_init(PatientList *pl);
void  plist_free(PatientList *pl);
void  plist_clear(PatientList *pl); /* mantém capacidade, limpa conteúdos */

/* Capacitação */
int   plist_reserve(PatientList *pl, size_t new_cap); /* 0 ok, -1 erro */
int   plist_shrink_to_fit(PatientList *pl); /* reduz cap para size */

/* Acesso / busca */
int   plist_find_index(const PatientList *pl, const char *id); /* -1 se não achar */
Patient* plist_get(PatientList *pl, const char *id); /* NULL se não achar */
Patient* plist_get_by_index(PatientList *pl, size_t idx); /* NULL se idx inválido */

/* Mutação */
int   plist_insert(PatientList *pl, const char *id, const char *name); /* 0 ok, -1 dup, -2 mem */
int   plist_remove(PatientList *pl, const char *id); /* 0 ok, -1 não achou */
int   plist_update_name(PatientList *pl, const char *id, const char *new_name); /* 0 ok, -1 não achou */
int   plist_set_called(PatientList *pl, const char *id, bool called); /* 0 ok, -1 não achou */

/* Debug / impressão */
void  plist_print(const PatientList *pl);

#endif