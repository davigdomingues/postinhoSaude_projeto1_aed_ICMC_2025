#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#if defined(_WIN32)
/* Compatibilidade: declare setenv para que módulos que chamem setenv() linkem
   com a implementação fornecida em util.c no Windows. */
int setenv(const char *name, const char *value, int overwrite);
#endif

/* Configuração de locale e console (UTF-8) — mover do main para util */
void util_setup_locale(void);

/* Utilitários de I/O locais e timestamp.
 *
 * Observações:
 * - Funções aqui são utilitárias para leitura segura do stdin e formatação de timestamps.
 * - Não fazem persistência nem I/O em ficheiros; usadas por main.c e outros módulos.
 */

/* Lê uma linha de stdin para 'buf' com no máximo 'size' bytes (incluindo '\0').
   Remove o terminador de linha ('\n' e '\r') se presente.
   Em caso de EOF ou erro, coloca uma string vazia em buf. */
void read_line(char *buf, size_t size);

/* Retorna 1 se a última chamada a read_line foi truncada (linha de entrada maior que buffer),
   0 caso contrário. A função também garante que o restante da linha no stdin foi descartado. */
int read_line_truncated(void);

/* Formata timestamp local em 'out' (inclui '\0'). Retorna 0 em sucesso, -1 se não foi possível. 
   Formato: "YYYY-MM-DD HH:MM" (usa localtime_r/localtime_s/localtime conforme plataforma). */
int format_timestamp(char *out, size_t out_size);

/* Funções de saída que garantem exibição correta de strings UTF-8 no Windows:
 * - util_printf: formata uma string (como printf) e imprime em UTF-8; retorna número de caracteres impressos.
 * - print_utf8: imprime diretamente uma string UTF-8 (sem formatação).
 */
int util_printf(const char *fmt, ...);
void print_utf8(const char *s);

#endif