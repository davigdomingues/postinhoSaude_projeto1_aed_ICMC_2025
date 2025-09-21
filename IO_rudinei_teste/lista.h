#ifndef LISTA_H
	#define LISTA_H

	#include "item.h"
	                       
	typedef struct lista_ LISTA;

	LISTA *LISTA_criar(bool ordenada);
	bool LISTA_inserir(LISTA *lista, ITEM *item);
	ITEM *LISTA_remover(LISTA *lista, int chave);
	ITEM *LISTA_remover_inicio(LISTA *lista);
	ITEM *LISTA_remover_fim(LISTA *lista);
	bool LISTA_apagar(LISTA **lista);
	int LISTA_tamanho(LISTA *lista);
	bool LISTA_vazia(LISTA *lista);
	bool LISTA_cheia(LISTA *lista);
	void LISTA_imprimir(LISTA *lista);

#endif