#include <stdio.h>
#include "historico.h"

typedef struct elo ELO;

struct elo {
    ITEM* item;
    ELO* anterior;
};

struct pilha_ {
    ELO* topo;
    int tamanho;
};

PILHA *historico_criar(){
    PILHA *pilha = (PILHA *) malloc(sizeof(PILHA));

    if (pilha != NULL) {
        pilha->topo = NULL;
        pilha->tamanho = 0;
    }
    return pilha;
}

void historico_apagar(PILHA** pilha) {
    ELO* aux;

    if(*pilha != NULL && !pilha_vazia(*pilha)) {
        while((*pilha)->topo != NULL) {
            aux = (*pilha)->topo;

            (*pilha)->topo = (*pilha)->topo->anterior;

            item_apagar(&aux->item);

            aux->anterior = NULL;

            free(aux);
            
            aux = NULL;
        }
    }
    free(*pilha);
    *pilha = NULL;
}

bool historico_empilhar(PILHA *pilha, ITEM *item){
    if(!historico_cheio(pilha)) {
        ELO* novo = (ELO *) malloc(sizeof(ELO));

        if (novo != NULL) {
            novo->item = item;

            novo->anterior = pilha->topo;

            pilha->topo = novo;

            pilha->tamanho++;

            return true;
        }
    }
    return false;
}

ITEM *historico_desempilhar(PILHA *pilha){
    if(pilha != NULL && !pilha_vazia(pilha)) {
        ELO* quebra = pilha->topo;
        
        ITEM* i = pilha->topo->item;

        pilha->topo = pilha->topo->anterior;

        quebra->anterior = NULL;
        
        free(quebra);
        
        quebra = NULL;

        pilha->tamanho--;

        return i;
    }
    return NULL;
}

bool historico_cheio(PILHA *pilha) {
    if(pilha != NULL) {
        return (pilha->tamanho == TAM_MAX) ? true : false;
    }
    return false;
}

bool historico_vazio(PILHA *pilha) {
    if(pilha != NULL) {
        return (pilha->tamanho == 0) ? true : false;
    }
    return false;
}

int historico_tamanho(PILHA *pilha) {
    if(pilha != NULL) {
        return pilha->tamanho;
    }
    return NULL;
}

ITEM* historico_topo(PILHA* pilha) {
    if(pilha != NULL && !pilha_vazia(pilha)) {
        return pilha->topo->item;
    }
    return NULL;
}

void historico_consultar(PILHA *pilha) {
    int n = pilha->tamanho;

    ITEM array[n];

    for(int i = 0; i < n; i++) {
        array[i] = historico_desempilhar(pilha);
    }

    for(int i = n-1; i >= 0; i--) {
        historico_empilhar(pilha, array[i]);
    }
}