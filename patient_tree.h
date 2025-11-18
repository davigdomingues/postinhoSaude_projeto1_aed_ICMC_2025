#ifndef PATIENT_TREE_H
#define PATIENT_TREE_H
#include <stddef.h>
#include <stdbool.h>
#include "config.h"

typedef struct PatientNode PatientNode;

typedef struct {
    PatientNode *root;
    size_t size;
} PatientTree;

PatientTree *ptree_create(void);
void         ptree_destroy(PatientTree *t);

int  ptree_insert(PatientTree *t, const char *id, const char *name); /* 0 ok, -1 chave duplicada, -2 falha de memória */
int  ptree_remove(PatientTree *t, const char *id); /* 0 ok, -1 não achou */
int  ptree_set_called(PatientTree *t, const char *id, bool called);
int  ptree_is_called(const PatientTree *t, const char *id);

int  ptree_get_name(const PatientTree *t, const char *id, char *out, size_t out_size);
int  ptree_exists(const PatientTree *t, const char *id); /* 1 existe, 0 não existe */
size_t ptree_size(const PatientTree *t);

/* Percurso em ordem chamando callback(id,name,called,userdata) */
typedef void (*ptree_visit_fn)(const char*, const char*, bool, void*);
void ptree_inorder(const PatientTree *t, ptree_visit_fn fn, void *userdata);

#endif