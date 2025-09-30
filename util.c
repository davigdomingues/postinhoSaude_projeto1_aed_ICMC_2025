#include "util.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int last_truncated = 0;

/* Lê uma linha segura de stdin, garante terminação nula e remove CR/LF.
   Define last_truncated = 1 se a linha foi truncada (entrada maior que buffer). */
void read_line(char *buf, size_t size) {
    last_truncated = 0;

    if (size == 0 || !buf)
        return;

    if (!fgets(buf, (int)size, stdin)) {
        /* EOF ou erro: garantir string vazia */
        buf[0] = '\0';
        return;
    }

    /* Se fgets não encontrou '\n', e não atingiu EOF, a linha foi possivelmente truncada.
       Detectamos isso verificando se o buffer contém '\n'. */
    if (strchr(buf, '\n') == NULL) {
        /* A entrada pode ter sido truncada; descartar o restante da linha do stdin */
        int c;
        while ((c = getchar()) != EOF && c != '\n') { /* consume */ }
        last_truncated = 1;
    }

    /* remover terminadores de linha CR/LF do final (se houver) */
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
}

int read_line_truncated(void) {
    return last_truncated;
}