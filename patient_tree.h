#ifndef PATIENT_TREE_H
#define PATIENT_TREE_H
#include <stddef.h>
#include <stdbool.h>
#include "config.h"

/* TAD PatientTree (AVL):
 * - Armazena pacientes por ID com operações O(log n) (inserir, remover, buscar).
 * - Cada paciente contém: id, nome, histórico (History*), flags (called, discharged) e prioridade (1..5).
 * - Wrappers expõem manipulação de histórico associado.
 */

typedef struct PatientNode PatientNode;

/* Estrutura opaca da árvore (definida em patient_tree.c). */
typedef struct {
    PatientNode *root;
    size_t size;
} PatientTree;

/* Cria uma árvore vazia.
 * Retorno: ponteiro válido em sucesso; NULL em erro de memória.
 */
PatientTree *ptree_create(void);

/* Destrói a árvore e todos os nós/recursos associados (inclui históricos).
 * Aceita NULL (noop).
 */
void ptree_destroy(PatientTree *t);

/* Operações principais
 * - ptree_insert: insere (0 ok, -1 duplicado, -2 memória/entrada inválida)
 * - ptree_remove: remove por id (0 ok, -1 não encontrado)
 * - ptree_set_called / ptree_is_called: marca/consulta flag de chamado
 * - ptree_set_priority / ptree_get_priority: define/obtém prioridade (1..5)
 */
int  ptree_insert(PatientTree *t, const char *id, const char *name); /* 0 ok, -1 chave duplicada, -2 falha de memória */
int  ptree_remove(PatientTree *t, const char *id); /* 0 ok, -1 não achou */
int  ptree_set_called(PatientTree *t, const char *id, bool called);

/* Consulta flag "chamado".
 * Parâmetros: t, id.
 * Retorno: 1 se chamado; 0 se não chamado ou inexistente.
 */
int  ptree_is_called(const PatientTree *t, const char *id);

/* Define prioridade (1..5) do paciente.
 * Parâmetros: t, id, pri (1..5).
 * Retorno: 0 sucesso; -1 se id inválido/inexistente ou prioridade fora do intervalo.
 */
int  ptree_set_priority(PatientTree *t, const char *id, int pri);

/* Obtém prioridade do paciente.
 * Parâmetros: t, id.
 * Retorno: prioridade (1..5) em sucesso; 5 (default) se inválido/inexistente.
 */
int  ptree_get_priority(const PatientTree *t, const char *id);

/* Consultas */
int ptree_get_name(const PatientTree *t, const char *id, char *out, size_t out_size);
int ptree_exists(const PatientTree *t, const char *id); /* 1 existe, 0 não existe */
size_t ptree_size(const PatientTree *t);

/* Percorre a árvore em ordem (inorder) e chama callback para cada nó.
 * Parâmetros: t, fn (callback com assinatura void(id,name,called,userdata)), userdata (opaque).
 */
typedef void (*ptree_visit_fn)(const char*, const char*, bool, void*);
void ptree_inorder(const PatientTree *t, ptree_visit_fn fn, void *userdata);

/* Wrappers de histórico (associado ao nó do paciente):
 * - Consultam/inserem/removem entradas no histórico do paciente referenciado por id.
 */

/* Verifica se histórico do paciente está cheio.
 * Parâmetros: t, id.
 * Retorno: 1 cheio; 0 não cheio/erro.
 */
int ptree_history_is_full(const PatientTree *t, const char *id);

/* Adiciona uma entrada ao histórico do paciente.
 * Parâmetros: t, id, proc (descrição, até PROC_MAX_LEN).
 * Retorno: 0 sucesso; -1 erro (id inválido/histórico cheio).
 */
int ptree_history_push(PatientTree *t, const char *id, const char *proc);

/* Remove a última entrada do histórico do paciente e copia para out.
 * Parâmetros: t, id, out, out_size.
 * Retorno: 0 sucesso; -1 erro (histórico vazio/id inválido/parâmetros).
 */
int ptree_history_pop(PatientTree *t, const char *id, char *out, size_t out_size);

/* Obtém o tamanho do histórico do paciente.
 * Parâmetros: t, id.
 * Retorno: número de entradas; 0 em inválido/sem histórico.
 */
int ptree_history_size_by_id(const PatientTree *t, const char *id);

/* Lê entrada do histórico por índice para out (sem remover).
 * Parâmetros: t, id, hist_idx (0..size-1), out, out_size.
 * Retorno: 0 sucesso; -1 erro (índice inválido/id inválido/parâmetros).
 */
int ptree_history_get_by_id(const PatientTree *t, const char *id, int hist_idx, char *out, size_t out_size);

/* Alta (discharged): flag de alta persistente. */

/* Define flag "discharged" (alta concedida) do paciente.
 * Parâmetros: t, id, discharged (true/false).
 * Retorno: 0 sucesso; -1 id inválido/inexistente.
 */
int ptree_set_discharged(PatientTree *t, const char *id, bool discharged);

/* Consulta flag "discharged" (alta).
 * Parâmetros: t, id.
 * Retorno: 1 alta concedida; 0 não concedida ou inexistente.
 */
int ptree_is_discharged(const PatientTree *t, const char *id);

#endif