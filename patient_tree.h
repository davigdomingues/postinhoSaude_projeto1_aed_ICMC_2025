#ifndef PATIENT_TREE_H
#define PATIENT_TREE_H
#include <stddef.h>
#include <stdbool.h>
#include "config.h"

typedef struct PatientTree PatientTree;

/* Ciclo de vida da árvore */
PatientTree *ptree_create(void);
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
int  ptree_is_called(const PatientTree *t, const char *id);
int  ptree_set_priority(PatientTree *t, const char *id, int pri);
int  ptree_get_priority(const PatientTree *t, const char *id);

/* Consultas */
int    ptree_get_name(const PatientTree *t, const char *id, char *out, size_t out_size);
int    ptree_exists(const PatientTree *t, const char *id); /* 1 existe, 0 não existe */
size_t ptree_size(const PatientTree *t);

/* Percurso inorder (ordem por id) com callback(id,name,called,userdata) */
typedef void (*ptree_visit_fn)(const char*, const char*, bool, void*);
void ptree_inorder(const PatientTree *t, ptree_visit_fn fn, void *userdata);

/* Wrappers de histórico: permitem manipular History via árvore */
int ptree_history_is_full(const PatientTree *t, const char *id);
int ptree_history_push(PatientTree *t, const char *id, const char *proc);
int ptree_history_pop(PatientTree *t, const char *id, char *out, size_t out_size);
int ptree_history_size_by_id(const PatientTree *t, const char *id);
int ptree_history_get_by_id(const PatientTree *t, const char *id, int hist_idx, char *out, size_t out_size);

/* Alta (discharge): flag pública */
int ptree_set_discharged(PatientTree *t, const char *id, bool discharged);
int ptree_is_discharged(const PatientTree *t, const char *id);

#endif