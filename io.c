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
    if (!s) return;

    size_t n = strlen(s);

    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
}


int io_save(const char *path, const PatientList *pl, const Queue *q) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;

    size_t n_pat = plist_size(pl);
    fprintf(f, "%zu\n", n_pat);

    for (size_t i = 0; i < n_pat; ++i) {
        char id[MAX_ID_LEN + 2];
        char name[MAX_NAME_LEN + 2];
        if (plist_get_id_by_index(pl, i, id, sizeof(id)) != 0) { fclose(f); return -1; }
        if (plist_get_name_by_index(pl, i, name, sizeof(name)) != 0) { fclose(f); return -1; }

        fprintf(f, "%s\n%s\n", id, name);

        int n_hist = plist_history_size_by_index(pl, i);
        fprintf(f, "%d\n", n_hist);
        for (int k = 0; k < n_hist; ++k) {
            char line[PROC_MAX_LEN + 2];
            if (plist_history_get_by_index(pl, i, k, line, sizeof(line)) != 0) { fclose(f); return -1; }
            fprintf(f, "%s\n", line);
        }

        int called = plist_is_called(pl, id);
        fprintf(f, "%d\n", called ? 1 : 0);
    }

    int qsize = queue_size(q);
    fprintf(f, "%d\n", qsize);
    for (int i = 0; i < qsize; ++i) {
        char id[MAX_ID_LEN + 2];
        if (queue_get_id_by_index(q, i, id, sizeof(id)) != 0) continue;
        fprintf(f, "%s\n", id);
    }

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

        for (int k = 0; k < n_hist; ++k) {
            if (!fgets(line, sizeof(line), f)) {
                fclose(f);
                return -2;
            }

            chomp(line);
            /* usar wrapper para adicionar histórico */
            if (plist_history_push(pl, id, line) != 0) {
                /* falha ao inserir histórico -> ignoramos item e continuamos */
            }
        }

        int called_flag = 0;

        if (fscanf(f, "%d\n", &called_flag) != 1) {
            fclose(f);
            return -2;
        }

        plist_set_called(pl, id, called_flag ? true : false);
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
        (void)queue_enqueue(q, id); /* ignora overflow */
    }

    fclose(f);
    return 0;
}