
#ifndef FILA_H
	#define FILA_H
	#include "item.h"

	typedef struct fila_ FILA;

	FILA *FILA_criar(void);
	bool FILA_inserir(FILA *fila, ITEM *item);
	ITEM *FILA_remover(FILA *fila);
	void FILA_apagar(FILA **fila);
	ITEM *FILA_frente(FILA *fila);
	int FILA_tamanho(FILA *fila);
	bool FILA_vazia(FILA *fila);
	bool FILA_cheia(FILA *fila);
	void FILA_imprimir(FILA *fila);

	  
#endif
