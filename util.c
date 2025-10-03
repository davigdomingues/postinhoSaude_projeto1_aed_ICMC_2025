#include "util.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

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

/* Formata timestamp local em out. Retorna 0 em sucesso, -1 em erro */
int format_timestamp(char *out, size_t out_size) {
    if (!out || out_size == 0) return -1;
    out[0] = '\0';

    time_t t = time(NULL);
    if (t == (time_t)-1) return -1;

#if defined(_WIN32) || defined(_MSC_VER)
    struct tm tmbuf;
    if (localtime_s(&tmbuf, &t) != 0) return -1;
    if (strftime(out, out_size, "%Y-%m-%d %H:%M", &tmbuf) == 0) return -1;
    return 0;
#elif defined(__unix__) || defined(__APPLE__)
    struct tm tmbuf;
    if (localtime_r(&t, &tmbuf) == NULL) return -1;
    if (strftime(out, out_size, "%Y-%m-%d %H:%M", &tmbuf) == 0) return -1;
    return 0;
#else
    struct tm *tmp = localtime(&t);
    if (!tmp) return -1;
    if (strftime(out, out_size, "%Y-%m-%d %H:%M", tmp) == 0) return -1;
    return 0;
#endif
}

/* Implementações de util.h
 *
 * Observações:
 * - read_line() e read_line_truncated() garantem comportamento consistente e descarte do restante da linha.
 * - format_timestamp() usa apis seguras (localtime_r/localtime_s) conforme plataforma.
 * - Este módulo não faz persistência em disco.
 */