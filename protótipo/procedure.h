#ifndef PROCEDURE_H
#define PROCEDURE_H

#include <stdio.h>

typedef struct procedure_ PROCEDURE;

PROCEDURE *procedure_create(void);
void procedure_destroy(PROCEDURE* procedure);
void procedure_print(PROCEDURE* procedure);

#endif