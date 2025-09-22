#include "IO.h"
#include <stdio.h>
#include <stdlib.h>

#define LISTA_FILENAME "lista_itens.bin"
#define FILA_FILENAME "fila_itens.bin"

bool SAVE(LISTA *lista, FILA *fila) {
    if(!lista || !fila) 
        return false;
    
    ITEM *it; // Variável auxiliar 

    // Salvando os itens da lista

    FILE *fp_lista = fopen(LISTA_FILENAME, "wb");
    if(!fp_lista)
        return false;

    it = LISTA_remover_inicio(lista);
    int chave;
    while(it != NULL) { // Se mantém no while enquanto a lista não estiver vazia
        // Escreve a chave no arquivo
        chave = item_get_chave(it);
        fwrite(&chave, sizeof(int), 1, fp_lista);
        // Apaga o item removido
        item_apagar(&it);
        // Atualiza a variável auxiliar
        it = LISTA_remover_inicio(lista);
    }
    // Libera memória
    LISTA_apagar(&lista);
    fclose(fp_lista); fp_lista = NULL;

    // Salvando os itens da fila
    FILE *fp_fila = fopen(FILA_FILENAME, "wb");
    if(!fp_fila) {
        fprintf(stderr, "Erro parcial: lista salva e destruída, mas houve falha ao abrir o arquivo da fila '%s' para escrita.\n", FILA_FILENAME);
        return false;
    }

    it = FILA_remover(fila);
    while(it != NULL) { // Se mantém no while enquanto a fila não estiver vazia
        // Escreve a chave no arquivo
        chave = item_get_chave(it);
        fwrite(&chave, sizeof(int), 1, fp_fila);
        // Apaga o item removido
        item_apagar(&it);
        // Atualiza a variável auxiliar
        it = FILA_remover(fila);
    }
    // Libera memória
    FILA_apagar(&fila);
    fclose(fp_fila); fp_fila = NULL;

    return true;
}

bool LOAD(LISTA **lista, FILA **fila) {
    if(!lista || !fila || !*lista || !*fila) 
        return false;

    int chave; // Variável auxiliar

    // Carregando os itens do arquivo na lista
    FILE *fp_lista = fopen(LISTA_FILENAME, "rb");
    if(!fp_lista)
        return false;

    // Lê as chaves até o fim do arquivo
    while(fread(&chave, sizeof(int), 1, fp_lista) == 1) {
        ITEM *it = item_criar(chave);
        LISTA_inserir(*lista, it);
    }
    fclose(fp_lista); // Libera memória

    // Carregando os itens do arquivo na fila
    FILE *fp_fila = fopen(FILA_FILENAME, "rb");
    if(!fp_fila)
        return false;

    // Lê as chaves até o fim do arquivo
    while(fread(&chave, sizeof(int), 1, fp_fila) == 1) {
        ITEM *it = item_criar(chave);
        FILA_inserir(*fila, it);
    }
    fclose(fp_fila); // Libera memória

    return true;
}