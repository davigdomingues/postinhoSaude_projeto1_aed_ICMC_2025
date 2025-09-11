#ifndef PROCEDIMENTO_H
#include <stdio.h>

#define PROCEDIMENTO_H

typedef struct procedimento_ PROCEDIMENTO;

PROCEDIMENTO *procedimento_criar();
void procedimento_apagar(PROCEDIMENTO* procedimento);
void procedimento_imprimir(PROCEDIMENTO* procedimento);

#endif