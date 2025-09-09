#include "procedimento.h"

struct procedimento_ {
    char *str;
};

PROCEDIMENTO *procedimento_criar() {
    PROCEDIMENTO *procedimento = (PROCEDIMENTO *) malloc(sizeof(PROCEDIMENTO));

    if(procedimento != NULL) {
        procedimento->str = (char *) malloc(101 * sizeof(char));

        fgets(procedimento->str, 100, stdin);
    }
}