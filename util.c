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
    /* define locale a partir do ambiente; preferir UTF-8 quando disponível */
    setlocale(LC_ALL, "");
#if defined(_WIN32)
    /* força codepage do console para UTF-8 no Windows (melhora exibição de acentos no cmd.exe) */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    /* tenta assegurar que há um locale UTF-8 quando não definido (melhora exibição em terminais Unix) */
    if (!getenv("LANG") && !getenv("LC_ALL")) {
        /* esforço simples, não sobrescreve configuração do utilizador se já existir */
        setenv("LC_ALL", "en_US.UTF-8", 0);
        setlocale(LC_ALL, "");
    }
#endif
}

/* Implementações de util.h
 *
 * Observações:
 * - read_line() e read_line_truncated() garantem comportamento consistente e descarte do restante da linha.
 * - format_timestamp() usa apis seguras (localtime_r/localtime_s) conforme plataforma.
 * - Este módulo não faz persistência em disco.
 */

#if defined(_WIN32)
/* Implementação compatível de setenv para o CRT do Windows.
   Retorna 0 em sucesso, -1 em erro. */
int setenv(const char *name, const char *value, int overwrite) {
    if (!name || !value) return -1;
    if (!overwrite && getenv(name) != NULL) return 0;
    /* _putenv_s retorna 0 em sucesso */
    return _putenv_s(name, value) == 0 ? 0 : -1;
}
#endif