#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "procedure.h"
#include "config.h"
#include "util.h" /* usar read_line/read_line_truncated */

struct procedure_ {
    char *str;
};

PROCEDURE *procedure_create(void) {
    PROCEDURE *procedure = malloc(sizeof(PROCEDURE));
    if (procedure == NULL) {
        return NULL;
    }

    procedure->str = malloc((PROC_MAX_LEN + 1) * sizeof(char));
    if (procedure->str == NULL) {
        // malloc falhou, libera-se procedure e se retorna NULL
        free(procedure);
        return NULL;
    }

    /* Usar read_line para comportamento consistente com o restante do projeto */
    read_line(procedure->str, PROC_MAX_LEN + 1);
    if (read_line_truncated()) {
        /* já descartamos o restante da linha em read_line; opcional: indicar truncamento com '...' */
        size_t len = strlen(procedure->str);
        if (len >= 3) {
            if (len > PROC_MAX_LEN - 3) procedure->str[PROC_MAX_LEN - 3] = '\0';
            strcat(procedure->str, "...");
        }
    }

    return procedure;
}

void procedure_destroy(PROCEDURE* procedure) {
    if (procedure != NULL) {
        free(procedure->str);
        procedure->str = NULL;
        free(procedure);
    }
}

void procedure_print(PROCEDURE* procedure) {
    if (procedure != NULL && procedure->str != NULL) {
        printf("%s\n", procedure->str);
    }
}