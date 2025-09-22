#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "procedure.h"
#include "config.h"

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

    if (fgets(procedure->str, PROC_MAX_LEN + 1, stdin) != NULL) {
        size_t len = strlen(procedure->str);
        if (len > 0 && procedure->str[len-1] == '\n')
            procedure->str[len-1] = '\0';
    } else {
        procedure->str[0] = '\0';
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