/* Estrutura: Árvore AVL de pacientes (chave = id).
 * Objetivo: Fornecer buscas, inserções e remoções O(log n) mantendo histórico associado.
 * Notas:
 *  - Cada nó guarda Patient com ponteiro para History (TAD opaca).
 *  - Rebalanceamento padrão AVL (rotações simples e duplas).
 *  - Remoção troca dados com sucessor in-order para simplificar lógica.
 */

#include <stdlib.h>
#include <string.h>
#include "patient_tree.h"
#include "history.h"
#include "util.h"

/* Estrutura interna do paciente armazenado no nó AVL. */
typedef struct Patient {
    char id[MAX_ID_LEN+1];
    char name[MAX_NAME_LEN+1];
    History *hist;   /* histórico de procedimentos (pilha fixa) */
    bool called;     /* flag se foi chamado */
} Patient;

/* Nó AVL: contém dados + filhos + altura para balanceamento. */
struct PatientNode {
    Patient data;
    struct PatientNode *left, *right;
    int height;
};

/* Retorna altura do nó (0 se NULL). */
static int node_height(PatientNode *n){ 
    return n ? n->height : 0; 
}

/* max auxiliar simples. */
static int max(int a,int b){ 
    return a>b?a:b; 
}

/* Atualiza altura com base nas subárvores. */
static void update_height(PatientNode *n){
    if(n) n->height = 1 + max(node_height(n->left), node_height(n->right));
}

/* Rotação simples à direita (corrige caso LL). */
static PatientNode *rotate_right(PatientNode *y){
    PatientNode *x = y->left;
    PatientNode *T2 = x->right;
    /* Reencaixa ponteiros */
    x->right = y;
    y->left = T2;
    /* Atualiza alturas após mudança */
    update_height(y); update_height(x);
    return x;
}

/* Rotação simples à esquerda (corrige caso RR). */
static PatientNode *rotate_left(PatientNode *x){
    PatientNode *y = x->right;
    PatientNode *T2 = y->left;
    y->left = x;
    x->right = T2;
    update_height(x); update_height(y);
    return y;
}

/* Fator de balanceamento: altura(left) - altura(right). */
static int balance_factor(PatientNode *n){
    return n ? node_height(n->left) - node_height(n->right) : 0;
}

/* Cria novo nó:
 * - Copia id e name (trunca com terminador garantido)
 * - Inicializa histórico
 * - Altura inicial = 1
 */
static PatientNode *new_node(const char *id, const char *name){
    PatientNode *n = (PatientNode*)malloc(sizeof(*n));
    if(!n) return NULL;
    strncpy(n->data.id,id,MAX_ID_LEN); n->data.id[MAX_ID_LEN]='\0';
    strncpy(n->data.name,name,MAX_NAME_LEN); n->data.name[MAX_NAME_LEN]='\0';
    n->data.hist = history_create();
    n->data.called = false;
    n->left = n->right = NULL;
    n->height = 1;
    if(!n->data.hist){ free(n); return NULL; }
    return n;
}

/* Inserção AVL:
 * 1. Inserção recursiva padrão por chave.
 * 2. Atualiza altura.
 * 3. Verifica fator de balanceamento.
 * 4. Aplica uma das quatro rotações conforme padrão (LL, RR, LR, RL).
 * Códigos de retorno:
 *   *rcode = 0 sucesso, -1 duplicado, -2 memória.
 */
static PatientNode *avl_insert(PatientNode *root, const char *id, const char *name, int *rcode){
    if(!root){
        PatientNode *nn = new_node(id,name);
        *rcode = nn?0:-2;
        return nn;
    }
    int cmp = strcmp(id, root->data.id);
    if(cmp == 0){ *rcode = -1; return root; } /* chave duplicada */
    if(cmp < 0)
        root->left = avl_insert(root->left,id,name,rcode);
    else
        root->right = avl_insert(root->right,id,name,rcode);

    update_height(root);
    int bf = balance_factor(root);

    /* Casos de rotação (comparação adicional define tipo) */
    if(bf > 1 && strcmp(id, root->left->data.id) < 0) /* LL */
        return rotate_right(root);
    if(bf < -1 && strcmp(id, root->right->data.id) > 0) /* RR */
        return rotate_left(root);
    if(bf > 1 && strcmp(id, root->left->data.id) > 0){ /* LR */
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }
    if(bf < -1 && strcmp(id, root->right->data.id) < 0){ /* RL */
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }
    return root;
}

/* Retorna nó mínimo (mais à esquerda) — usado para sucessor em remoção. */
static PatientNode *min_node(PatientNode *n){
    while(n && n->left) n = n->left;
    return n;
}

/* Remoção AVL:
 * 1. Busca recursiva por id.
 * 2. Libera histórico ao remover nó alvo.
 * 3. Caso 0 ou 1 filho: retorna filho direto.
 * 4. Caso 2 filhos: troca dados com sucessor in-order para simplificar lógica.
 * 5. Rebalanceia subárvore afetada.
 * *rcode = 0 sucesso, -1 não encontrado.
 */
static PatientNode *avl_remove(PatientNode *root, const char *id, int *rcode){
    if(!root){ *rcode = -1; return NULL; }
    int cmp = strcmp(id, root->data.id);
    if(cmp < 0)
        root->left = avl_remove(root->left,id,rcode);
    else if(cmp > 0)
        root->right = avl_remove(root->right,id,rcode);
    else {
        /* encontrado: NÃO destruir history aqui, pois em caso de dois filhos
           haverá um swap de dados e o histórico deve ser destruído apenas quando
           o nó é realmente liberado. */
        if(!root->left || !root->right){
            /* caso simples: 0 ou 1 filho */
            PatientNode *tmp = root->left ? root->left : root->right;
            /* destruir histórico do nó que será removido */
            if(root->data.hist) history_destroy(root->data.hist);
            free(root);
            *rcode = 0;
            return tmp;
        } else {
            /* dois filhos: usa sucessor à direita */
            PatientNode *succ = min_node(root->right);
            /* troca dados (incluindo ponteiro hist) */
            Patient tmpd = root->data;
            root->data = succ->data;
            succ->data = tmpd;
            /* remove sucessor com o id que estava originalmente em root (tmpd.id) */
            root->right = avl_remove(root->right, tmpd.id, rcode);
        }
    }
    if(!root) return NULL;
    update_height(root);
    int bf = balance_factor(root);

    /* Rebalance pós-remoção (mesma lógica dos casos padrão) */
    if(bf > 1 && balance_factor(root->left) >= 0)
        return rotate_right(root);
    if(bf > 1 && balance_factor(root->left) < 0){
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }
    if(bf < -1 && balance_factor(root->right) <= 0)
        return rotate_left(root);
    if(bf < -1 && balance_factor(root->right) > 0){
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }
    return root;
}

/* Busca iterativa por id (O(log n)). */
static PatientNode *find_node(PatientNode *root, const char *id){
    while(root){
        int cmp = strcmp(id, root->data.id);
        if(cmp == 0) return root;
        root = (cmp < 0) ? root->left : root->right;
    }
    return NULL;
}

/* Liberação recursiva pós-ordem (garante destruir históricos). */
static void destroy_rec(PatientNode *n){
    if(!n) return;
    destroy_rec(n->left);
    destroy_rec(n->right);
    if(n->data.hist) history_destroy(n->data.hist);
    free(n);
}

/* Percurso inorder chamando callback para cada paciente (ordem por id). */
static void inorder_rec(const PatientNode *n, ptree_visit_fn fn, void *ud){
    if(!n) return;
    inorder_rec(n->left,fn,ud);
    fn(n->data.id, n->data.name, n->data.called, ud);
    inorder_rec(n->right,fn,ud);
}

/* Cria árvore vazia (root=NULL, size=0). */
PatientTree *ptree_create(void){
    PatientTree *t = (PatientTree*)malloc(sizeof(*t));
    if(!t) return NULL;
    t->root = NULL; t->size = 0;
    return t;
}

/* Destrói toda a estrutura (nós + históricos). */
void ptree_destroy(PatientTree *t){
    if(!t) return;
    destroy_rec(t->root);
    free(t);
}

/* Inserção pública — delega para avl_insert e atualiza size em sucesso. */
int ptree_insert(PatientTree *t, const char *id, const char *name){
    if(!t || !id || !*id || !name || !*name) return -2;
    int rc = 0;
    t->root = avl_insert(t->root,id,name,&rc);
    if(rc == 0) t->size++;
    return rc;
}

/* Remoção pública — atualiza size se rc==0. */
int ptree_remove(PatientTree *t, const char *id){
    if(!t || !id) return -1;
    int rc = -1;
    t->root = avl_remove(t->root,id,&rc);
    if(rc == 0) t->size--;
    return rc;
}

/* Marca paciente como chamado. */
int ptree_set_called(PatientTree *t, const char *id, bool called){
    if(!t) return -1;
    PatientNode *n = find_node(t->root,id);
    if(!n) return -1;
    n->data.called = called;
    return 0;
}

/* Consulta flag 'called' (1 chamado, 0 não chamado / inexistente). */
int ptree_is_called(const PatientTree *t, const char *id){
    if(!t || !id) return 0;
    PatientNode *n = find_node((PatientNode*)t->root,id);
    return n && n->data.called ? 1 : 0;
}

/* Obtém nome pelo id (cópia segura para out). */
int ptree_get_name(const PatientTree *t, const char *id, char *out, size_t out_size){
    if(!t || !id || !out || out_size==0) return -1;
    PatientNode *n = find_node((PatientNode*)t->root,id);
    if(!n) return -1;
    strncpy(out,n->data.name,out_size-1);
    out[out_size-1]='\0';
    return 0;
}

/* Verifica existência (1) ou ausência (0). */
int ptree_exists(const PatientTree *t, const char *id){
    return t && id && find_node((PatientNode*)t->root,id) ? 1 : 0;
}

/* Retorna número de pacientes. */
size_t ptree_size(const PatientTree *t){ return t ? t->size : 0; }

/* Percurso inorder com callback externo. */
void ptree_inorder(const PatientTree *t, ptree_visit_fn fn, void *userdata){
    if(!t || !fn) return;
    inorder_rec(t->root, fn, userdata);
}

/* Histórico: wrappers que acessam History* armazenado no nó correspondente */
int ptree_history_is_full(const PatientTree *t, const char *id) {
    if (!t || !id) return 0;
    PatientNode *n = find_node((PatientNode*)t->root, id);
    return (n && n->data.hist) ? (history_is_full(n->data.hist) ? 1 : 0) : 0;
}

int ptree_history_push(PatientTree *t, const char *id, const char *proc) {
    if (!t || !id || !proc) return -1;
    PatientNode *n = find_node((PatientNode*)t->root, id);
    return (n && n->data.hist) ? history_push(n->data.hist, proc) : -1;
}

int ptree_history_pop(PatientTree *t, const char *id, char *out, size_t out_size) {
    if (!t || !id || !out || out_size == 0) return -1;
    PatientNode *n = find_node((PatientNode*)t->root, id);
    return (n && n->data.hist) ? history_pop(n->data.hist, out, out_size) : -1;
}

int ptree_history_size_by_id(const PatientTree *t, const char *id) {
    if (!t || !id) return 0;
    PatientNode *n = find_node((PatientNode*)t->root, id);
    return (n && n->data.hist) ? history_size(n->data.hist) : 0;
}

int ptree_history_get_by_id(const PatientTree *t, const char *id, int hist_idx, char *out, size_t out_size) {
    if (!t || !id || !out || out_size == 0) return -1;
    PatientNode *n = find_node((PatientNode*)t->root, id);
    if (!n || !n->data.hist) return -1;
    return history_get_by_index(n->data.hist, hist_idx, out, out_size);
}