/* Explicação:
 *
 * Propósito:
 * - Serializar (salvar) e desserializar (carregar) as estruturas centrais do
 *   sistema: a lista de pacientes (PatientList) e a fila de espera (Queue).
 * - O ficheiro usa um formato textual simples, legível, onde cada
 *   item é escrito em linha separada seguindo uma ordem estrita definida por io_save
 *   e esperada por io_load.
 *
 * Funções exportadas (declaradas em io.h):
 * - io_save(const char *path, const PatientList *pl, const Queue *q)
 * - io_load(const char *path, PatientList *pl, Queue *q)
 *
 * Função auxiliar estática:
 * - chomp(char *s)
 *     Removes line terminators ('\n' and '\r') from the end of a string read with fgets.
 *
 * Dependências:
 * - Patient tem campos id[], name[] e History hist.
 * - History expõe `.top` (índice do topo, -1 se vazio) e `.items[]` (array de strings).
 * - PatientList e Queue expõem campos e funções utilizadas (plist_insert, plist_get,
 *   queue_enqueue) e tamanhos/capacidades (q->head, q->size, q->cap).
 * - As constantes MAX_ID_LEN, MAX_NAME_LEN, PROC_MAX_LEN são definidas em config.h
 *   e determinam os tamanhos dos buffers de leitura.
 */

#include <stdio.h>
#include <string.h>
#include "io.h"
#include "config.h"


static void chomp(char *s) {
    if (!s)
        return;

    size_t n = strlen(s);

    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
}


int io_save(const char *path, const PatientList *pl, const Queue *q) {
    FILE *f = fopen(path, "w");
    if (!f)
        return -1;

    fprintf(f, "%zu\n", pl->size);

    for (size_t i = 0; i < pl->size; ++i) {
        const Patient *p = &pl->data[i];

        fprintf(f, "%s\n", p->id);
        fprintf(f, "%s\n", p->name);
        fprintf(f, "%d\n", p->hist.top + 1);

        for (int k = 0; k <= p->hist.top; ++k)
            fprintf(f, "%s\n", p->hist.items[k]);

        fprintf(f, "%d\n", p->called ? 1 : 0);
    }

    fprintf(f, "%d\n", q->size);

    for (int i = 0, idx = q->head; i < q->size; ++i, idx = (idx + 1) % q->cap)
        fprintf(f, "%s\n", q->ids[idx]);

    fclose(f);
    return 0;
}


int io_load(const char *path, PatientList *pl, Queue *q) {
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    size_t n_pat = 0;

    if (fscanf(f, "%zu\n", &n_pat) != 1) {
        fclose(f);
        return -2;
    }

    for (size_t i = 0; i < n_pat; ++i) {
        char id[MAX_ID_LEN + 2];
        char name[MAX_NAME_LEN + 2];
        char line[PROC_MAX_LEN + 2];

        if (!fgets(id, sizeof(id), f)) {
            fclose(f);
            return -2;
        }

        if (!fgets(name, sizeof(name), f)) {
            fclose(f);
            return -2;
        }

        chomp(id);
        chomp(name);

        if (plist_insert(pl, id, name) < 0) {
            fclose(f);
            return -2;
        }

        int n_hist = 0;

        if (fscanf(f, "%d\n", &n_hist) != 1) {
            fclose(f);
            return -2;
        }

        Patient *p = plist_get(pl, id);

        for (int k = 0; k < n_hist; ++k) {
            if (!fgets(line, sizeof(line), f)) {
                fclose(f);
                return -2;
            }

            chomp(line);
            history_push(&p->hist, line);
        }

        int called_flag = 0;

        if (fscanf(f, "%d\n", &called_flag) != 1) {
            fclose(f);
            return -2;
        }

        p->called = called_flag ? true : false;
    }

    int m = 0;

    if (fscanf(f, "%d\n", &m) != 1) {
        fclose(f);
        return -2;
    }

    for (int i = 0; i < m; ++i) {
        char id[MAX_ID_LEN + 2];

        if (!fgets(id, sizeof(id), f)) {
            fclose(f);
            return -2;
        }

        chomp(id);

        if (queue_enqueue(q, id) != 0) {
            /* ignora overflow */
        }
    }

    fclose(f);
    return 0;
}