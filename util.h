#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* Lê uma linha de stdin para 'buf' com no máximo 'size' bytes (incluindo '\0').
   Remove o terminador de linha ('\n' e '\r') se presente.
   Em caso de EOF ou erro, coloca uma string vazia em buf. */
void read_line(char *buf, size_t size);

#endif
