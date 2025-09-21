#ifndef IO_H
	#define IO_H
	#include "item.h"
    #include "fila.h"
    #include "lista.h"
    #include <stdbool.h>

    bool SAVE(LISTA *lista, FILA *fila); // Salva os itens da lista e da fila em arquivos
    bool LOAD(LISTA **lista, FILA **fila); // Carrega os itens dos arquivos na lista e na fila
	  
#endif
