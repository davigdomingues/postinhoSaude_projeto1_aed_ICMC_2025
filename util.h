#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* Lê uma linha de stdin para 'buf' com no máximo 'size' bytes (incluindo '\0').
   Remove o terminador de linha ('\n' e '\r') se presente.
   Em caso de EOF ou erro, coloca uma string vazia em buf. */
void read_line(char *buf, size_t size);

/* Retorna 1 se a última chamada a read_line foi truncada (linha de entrada maior que buffer),
   0 caso contrário. A função também garante que o restante da linha no stdin foi descartado. */
int read_line_truncated(void);

#endif