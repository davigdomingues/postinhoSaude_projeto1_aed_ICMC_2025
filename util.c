#include "util.h"
#include <stdio.h>
#include <string.h>

void read_line(char *buf, size_t size) {
    if (size == 0) return;
    if (!fgets(buf, (int)size, stdin)) {
        /* EOF ou erro: garantir string vazia */
        buf[0] = '\0';
        return;
    }
    /* remover '\n' final se presente */
    size_t len = strlen(buf);
    if (len > 0) {
        if (buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
            len--;
        }
        if (len > 0 && buf[len - 1] == '\r') {
            buf[len - 1] = '\0';
        }
    }
}
