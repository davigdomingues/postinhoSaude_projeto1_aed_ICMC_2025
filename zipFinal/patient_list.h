/* Explicação:
 *
 * Funções principais:
 * - plist_init(PatientList *pl)
 *     Inicializa a estrutura definindo data = NULL, size = 0, cap = 0.
 *
 * - plist_free(PatientList *pl)
 *     Liberta a memória do array interno pl->data e zera campos; libera também
 *     os recursos alocados pelo histórico através da API pública de History.
 *
 * - plist_find_index(const PatientList *pl, const char *id)
 *     Procura pelo paciente com o ID fornecido; retorna índice (int) se encontrado
 *     ou -1 se não encontrado / id inválido. Implementação usa tabela hash quando disponível.
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
 *     Remove o paciente com o ID especificado. Libera o histórico associado via
 *     history_destroy() e atualiza a tabela interna (remoção O(1) por troca com último).
 *
 * - void plist_print(const PatientList *pl)
 *     Imprime uma lista simples de pacientes com ID, nome e número de procedimentos
 *     (obtido através da API pública de histórico).
 *
 * Observações:
 * - Este módulo implementa apenas a gestão em memória de pacientes e históricos.
 * - Persistência em disco é responsabilidade do módulo io (io_save/io_load).
 */

#ifndef PATIENT_LIST_H
#define PATIENT_LIST_H

#include <stddef.h>
#include <stdbool.h>
#include "config.h"
#include "history.h" /* Necessário para o tipo opaco */

typedef struct Patient Patient;
typedef struct PatientList PatientList;

/* Criar / destruir (alocam o TAD opaco) */
PatientList* plist_create(void);
void         plist_destroy(PatientList *pl);

/* Acesso / busca */
int   plist_find_index(const PatientList *pl, const char *id); /* -1 se não achar */

/* Accessors seguros (evitam expor campos internos) */
size_t plist_size(const PatientList *pl);
int    plist_get_id_by_index(const PatientList *pl, size_t idx, char *out, size_t out_size);
int    plist_get_name_by_index(const PatientList *pl, size_t idx, char *out, size_t out_size);
int    plist_get_name_by_id(const PatientList *pl, const char *id, char *out, size_t out_size);
int    plist_is_called(const PatientList *pl, const char *id);

/* Mutação */
int   plist_insert(PatientList *pl, const char *id, const char *name); /* 0 ok, -1 dup, -2 mem */
int   plist_remove(PatientList *pl, const char *id); /* 0 ok, -1 não achou */
int   plist_set_called(PatientList *pl, const char *id, bool called); /* 0 ok, -1 não achou */

/* Histórico: wrappers que manipulam o histórico por id/índice (evita expor History) */
int plist_history_is_full(const PatientList *pl, const char *id);
int plist_history_push(PatientList *pl, const char *id, const char *proc);
int plist_history_pop(PatientList *pl, const char *id, char *out, size_t out_size);
int plist_history_size_by_id(const PatientList *pl, const char *id);
int plist_history_get_by_id(const PatientList *pl, const char *id, int hist_idx, char *out, size_t out_size);

/* Histórico por índice do paciente (usado por io.c para serialização) */
int plist_history_size_by_index(const PatientList *pl, size_t patient_idx);
int plist_history_get_by_index(const PatientList *pl, size_t patient_idx, int hist_idx, char *out, size_t out_size);

/* Impressão */
void plist_print(const PatientList *pl);

#endif