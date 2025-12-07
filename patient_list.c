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
 * - Pressupõe existência de history_create/history_destroy e funções da API history_*
 *   para manipular o histórico; PatientList NÃO acessa campos internos do tipo
 *   History (usa apenas as funções públicas).
 *
 * Observações de integração:
 * - PatientList delega persistência em disco para io.c; funções aqui são apenas manipulação em memória.
 * - Ao remover um paciente chamamos history_destroy() para limpar o histórico antes de sobrescrever/soltar memória.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "patient_list.h"
#include "config.h"
#include "history.h"
/* Usamos util_printf para garantir exibição UTF-8 correta no Windows */
#include "util.h"

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
    /* tabela hash: array de ponteiros para entradas (separate chaining) */
    struct PlHashEntry **htable;
    size_t hcap; /* número de buckets */
};

/* Entrada da tabela hash que mapeia id -> índice no array de pacientes */
struct PlHashEntry {
    char *key; /* chave duplicada com strdup() */
    int idx;   /* índice no array pl->data */
    struct PlHashEntry *next;
};

/* djb2 string hash */
/* função de dispersão simples para strings (retorna um hash sem sinais) */
static unsigned long str_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}

/* inicializa a tabela hash interna (vazia) com 'buckets' baldes */
static int plist_htable_init(PatientList *pl, size_t buckets) {
    pl->htable = (struct PlHashEntry **)calloc(buckets, sizeof(struct PlHashEntry *));

    if (!pl->htable) 
        return -1;

    pl->hcap = buckets;
    
    return 0;
}

/* libera todas as entradas da tabela hash e o próprio array de baldes */
static void plist_htable_free(PatientList *pl) {
    if (!pl || !pl->htable) return;
    for (size_t i = 0; i < pl->hcap; ++i) {
        struct PlHashEntry *e = pl->htable[i];
        while (e) {
            struct PlHashEntry *n = e->next;
            free(e->key);
            free(e);
            e = n;
        }
    }
    free(pl->htable);
    pl->htable = NULL;
    pl->hcap = 0;
}

/* insere ou atualiza o mapeamento id -> indice no array de pacientes */
static int plist_htable_put(PatientList *pl, const char *key, int idx) {
    if (!pl || !pl->htable || !key) return -1;
    unsigned long h = str_hash(key) % pl->hcap;
    struct PlHashEntry *e = pl->htable[h];
    while (e) {
        if (strcmp(e->key, key) == 0) { e->idx = idx; return 0; }
        e = e->next;
    }
    /* chave duplicada primeiro para evitar libertar 'e' em caso de falha do strdup */
    char *dup = util_strdup(key); /* substitui strdup por versão C99 */
    if (!dup) return -1;

    e = (struct PlHashEntry *)malloc(sizeof(*e));
    if (!e) { free(dup); return -1; }

    e->key = dup;
    e->idx = idx;
    e->next = pl->htable[h];
    pl->htable[h] = e;
    return 0;
}

/* procura o indice associado ao id; retorna -1 se nao encontrar */
static int plist_htable_get(const PatientList *pl, const char *key) {
    if (!pl || !pl->htable || !key) return -1;
    unsigned long h = str_hash(key) % pl->hcap;
    struct PlHashEntry *e = pl->htable[h];
    while (e) {
        if (strcmp(e->key, key) == 0) return e->idx;
        e = e->next;
    }
    return -1;
}

/* remove o mapeamento para 'key' da tabela hash (liberta memoria associada) */
static void plist_htable_remove(PatientList *pl, const char *key) {
    if (!pl || !pl->htable || !key) return;
    unsigned long h = str_hash(key) % pl->hcap;
    struct PlHashEntry **pe = &pl->htable[h];
    while (*pe) {
        if (strcmp((*pe)->key, key) == 0) {
            struct PlHashEntry *rem = *pe;
            *pe = rem->next;
            free(rem->key);
            free(rem);
            return;
        }
        pe = &((*pe)->next);
    }
}

/* Helpers estáticos */
/* Expande a capacidade interna do array de pacientes.
   - Nova capacidade = cap*2 ou 16 se cap==0
   - Usa realloc; se realloc falhar, retorna -1 e pl permanece inalterado
   - Complexidade amortizada constante por insercao quando usado pelo loop de insercao */
static int plist_grow(PatientList *pl) {
    size_t ncap = pl->cap ? pl->cap * 2 : 16;
    Patient *nd = realloc(pl->data, ncap * sizeof(Patient));
    if (!nd) return -1;
    pl->data = nd;
    pl->cap = ncap;
    return 0;
}

/* Inicializa campos da estrutura PatientList (data=NULL, size=0, cap=0) */
static void plist_init(PatientList *pl) {
    pl->data = NULL;
    pl->size = 0;
    pl->cap = 0;
    pl->htable = NULL;
    pl->hcap = 0;
}

/* Liberta memórias associadas aos pacientes e aos historicos (history_destroy)
   e reseta os campos internos */
static void plist_free(PatientList *pl) {
    if (!pl) 
        return;

    /* libera tabela hash primeiro */
    plist_htable_free(pl);

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

/* Retorna ponteiro para paciente pelo id ou NULL se nao existir */
static Patient *plist_get(PatientList *pl, const char *id) {
    int idx = plist_find_index(pl, id);
    return idx < 0 ? NULL : &pl->data[idx];
}

/* Pesquisa linear por ID, retorna indice ou -1 */
int plist_find_index(const PatientList *pl, const char *id) {
    if (!pl || !id) return -1;
    /* usa tabela hash para busca O(1) se inicializada */
    if (pl->htable) {
        return plist_htable_get(pl, id);
    }
    /* fallback para busca linear */
    for (size_t i = 0; i < pl->size; ++i)
        if (strcmp(pl->data[i].id, id) == 0) return (int)i;
    return -1;
}

/* Insere novo paciente:
   - Valida parametros (pl, id, name)
   - Evita duplicatas (procura linear)
   - Garante espaço (plist_grow)
   - Copia id e name de forma segura com strncpy e terminação forçada
   - Cria um History opaco via history_create (centraliza alocacao) e inicializa o campo 'called' como false
   - Retorna codigos: 0 sucesso, -1 duplicado, -2 entrada invalida/erro memoria
*/
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

    /* garante que a tabela hash existe (capacidade inicial pequena) */
    if (!pl->htable) {
        /* escolhe um número de buckets (potência de dois) em relação ao tamanho esperado */
        size_t buckets = 256;
        (void)plist_htable_init(pl, buckets);
    }
    /* adiciona um mapeamento pelo id -> índice (pl->size-1) */
    (void)plist_htable_put(pl, p->id, (int)(pl->size - 1));
    
    return 0;
}

/* Remove paciente por ID:
   - Procura indice via plist_find_index
   - Libera o History associado usando history_destroy (se alocado)
   - Substitui o elemento removido pelo ultimo elemento do array (O(1))
   - Decrementa pl->size
   - Observacao: a ordem dos pacientes nao e preservada apos remocao */
int plist_remove(PatientList *pl, const char *id) {
    if (!pl || !id) 
        return -1;

    int idx = plist_find_index(pl, id);
    
    if (idx < 0) {
        return -1;
    }

    /* remove o histórico do paciente a ser excluído e substituir pelo último elemento */
    if (pl->data[idx].hist) {
        history_destroy(pl->data[idx].hist);
        pl->data[idx].hist = NULL;
    }
    /* remove da hash primeiro */
    plist_htable_remove(pl, pl->data[idx].id);

    if ((size_t)idx != pl->size - 1) {
        /* movimenta o último para idx e atualiza a hash para o id movido */
        pl->data[idx] = pl->data[pl->size - 1];
        /* atualiza a entrada da hash para o id movido para o novo índice */
        (void)plist_htable_put(pl, pl->data[idx].id, idx);
    }
    pl->size--;
    
    return 0;
}

/* Marca/desmarca o campo 'called' de um paciente */
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
    util_printf("Pacientes registrados: %lu\n", (unsigned long)pl->size);

    if (pl->size == 0) { 
        util_printf("Nenhum paciente registrado.\n");
        return; 
    }
    
    for (size_t i = 0; i < pl->size; ++i) {
        int hcount = pl->data[i].hist ? history_size(pl->data[i].hist) : 0;
        util_printf("- ID: %s | Nome: %s | Procedimentos: %d | Chamado: %s\n",
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
    /* inicializa tabela hash de forma preguiçosa na primeira inserção */
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
}