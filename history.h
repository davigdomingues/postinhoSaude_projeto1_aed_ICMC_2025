#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "config.h"

typedef struct {
    char items[HIST_MAX][PROC_MAX_LEN + 1];
    int top;
} History;

void history_init(History *h);
void history_free(History *h);

int history_push(History *h, const char *proc);

int history_pop(History *h, char *out, size_t out_size);

bool history_is_full(const History *h);
bool history_is_empty(const History *h);
int history_size(const History *h);

const char *history_top(const History *h);

void history_inspect(History *h);

#endif