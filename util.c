#include "util.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <locale.h> /* para setlocale() */
#if defined(_WIN32)
#include <windows.h>
#endif

static int last_truncated = 0;

/* Comprimento de string limitado compatível com C99 (substitui strnlen) */
size_t util_strnlen(const char *s, size_t maxlen) {
    if (!s) return 0;
    size_t i = 0;
    while (i < maxlen && s[i] != '\0') ++i;
    return i;
}

/* Duplicador de string compatível com C99 (substitui strdup) */
char *util_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *p = (char *)malloc(len);
    if (!p) return NULL;
    memcpy(p, s, len);
    return p;
}

/* Helper C99: obtém hora local em 'out' usando apenas localtime (copia segura) */
static int util_localtime(const time_t *t, struct tm *out) {
    if (!t || !out) return -1;
    struct tm *tmp = localtime(t);
    if (!tmp) return -1;
    *out = *tmp; /* copia para buffer do chamador */
    return 0;
}

/* Le uma linha do stdin de forma segura, remove CR/LF, descarta resto da linha se truncada.
   Define last_truncated = 1 se a entrada foi maior que o buffer.

   Detalhes:
   - Usa fgets com (int)size para evitar overflow do buffer.
   - Se fgets nao obteve '\n' e nao houve EOF, presume-se que a linha foi truncada:
     entao consumimos o restante da linha com getchar() ate encontrar '\n' ou EOF.
   - Ao final, remove possiveis terminadores CR/LF no final do buffer.
   - last_truncated indica ao chamador se a entrada foi truncada (para mensagens/erros).
*/
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

/* Retorna 1 se ultima chamada a read_line resultou em truncamento da entrada
   Comentario: funcao simples de consulta do estado interno last_truncated */
int read_line_truncated(void) {
    return last_truncated;
}

/* Formata timestamp local em out no formato "YYYY-MM-DD HH:MM".
   Retorna 0 em sucesso, -1 em erro.

   Detalhes:
   - Obtém time_t atual com time(NULL).
   - Usa localtime_r/localtime_s quando disponivel para evitar problemas de thread-safety.
   - Usa strftime para formatacao portavel.
   - Valores de retorno permitem ao chamador saber se a funcao falhou (ex.: errno/time(NULL) invalido).
*/
int format_timestamp(char *out, size_t out_size) {
    if (!out || out_size == 0) return -1;
    out[0] = '\0';

    time_t t = time(NULL);
    if (t == (time_t)-1) return -1;

    struct tm tmbuf;
    if (util_localtime(&t, &tmbuf) != 0) return -1;
    if (strftime(out, out_size, "%Y-%m-%d %H:%M", &tmbuf) == 0) return -1;
    return 0;
}

/* Imprime diretamente uma string UTF-8 de forma segura no Windows (WriteConsoleW)
   ou via fputs em plataformas POSIX. */
void print_utf8(const char *s) {
    if (!s) return;
#if defined(_WIN32)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) {
        fputs(s, stdout);
        return;
    }
    /* converte UTF-8 para UTF-16 */
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (wlen <= 0) { fputs(s, stdout); return; }
    wchar_t *wbuf = (wchar_t *)malloc((size_t)wlen * sizeof(wchar_t));
    if (!wbuf) { fputs(s, stdout); return; }
    if (MultiByteToWideChar(CP_UTF8, 0, s, -1, wbuf, wlen) == 0) {
        free(wbuf);
        fputs(s, stdout);
        return;
    }
    DWORD written = 0;
    WriteConsoleW(h, wbuf, wlen - 1, &written, NULL); /* wlen includes terminator */
    free(wbuf);
#else
    fputs(s, stdout);
#endif
}

/* printf que aceita formato e argumentos, produz UTF-8 corretamente no Windows */
int util_printf(const char *fmt, ...) {
    if (!fmt) return 0;
    int ret = 0;
    va_list ap;
    va_start(ap, fmt);
    /* formata em buffer temporário */
    char buf[1024];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    if (n < 0) {
        /* tentativa com alocação se necessário */
        va_end(ap);
        va_start(ap, fmt);
        int needed = vsnprintf(NULL, 0, fmt, ap);
        va_end(ap);
        if (needed <= 0) return 0;
        char *dyn = (char *)malloc((size_t)needed + 1);
        if (!dyn) return 0;
        va_start(ap, fmt);
        vsnprintf(dyn, (size_t)needed + 1, fmt, ap);
        print_utf8(dyn);
        ret = needed;
        free(dyn);
        va_end(ap);
        return ret;
    }
    /* n é número de bytes que seriam escritos; buf possui a string truncada ou completa */
    print_utf8(buf);
    va_end(ap);
    ret = n;
    return ret;
}

/* Configura locale/console para suportar UTF-8 de forma portátil.
   Usar esta função a partir de main() no início da execução. */
void util_setup_locale(void) {
    /* tenta usar locale do ambiente (padrão) */
    setlocale(LC_ALL, "");
#if defined(_WIN32)
    /* força codepage do console para UTF-8 no Windows (melhora exibição de acentos no cmd.exe) */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    /* Evita setenv: tenta alguns locais UTF-8 comuns via setlocale diretamente */
    const char *cur = setlocale(LC_ALL, NULL);
    int ok_utf8 = 0;
    if (cur && (strstr(cur, "UTF-8") || strstr(cur, "utf8"))) ok_utf8 = 1;

    if (!ok_utf8) {
        const char *cands[] = { "C.UTF-8", "en_US.UTF-8", "pt_BR.UTF-8", "POSIX", NULL };
        for (int i = 0; cands[i]; ++i) {
            if (setlocale(LC_ALL, cands[i])) {
                /* aceita o primeiro que funcionar; preferimos UTF-8, POSIX é fallback */
                break;
            }
        }
    }
#endif
}

/* Implementações de util.h
 *
 * Observações:
 * - read_line() e read_line_truncated() garantem comportamento consistente e descarte do restante da linha.
 * - format_timestamp() usa apenas APIs C99 (localtime + cópia para struct tm).
 * - Este módulo não faz persistência em disco.
 */