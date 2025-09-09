#ifndef PILHA_H
	#include <stdbool.h>
	#include "procedimento.h"
	
	#define PILHA_H
	#define TAM_MAX 10

	typedef struct pilha_ PILHA;

	PILHA *historico_criar(void);
	void historico_apagar(PILHA **pilha);

	bool historico_empilhar(PILHA *pilha, ITEM *item);
	ITEM *historico_desempilhar(PILHA *pilha);

	bool historico_cheio(PILHA *pilha);
	bool historico_vazio(PILHA *pilha);

	int historico_tamanho(PILHA* pilha);
	ITEM *historico_topo(PILHA *pilha);

	void historico_consultar(PILHA *pilha);
#endif