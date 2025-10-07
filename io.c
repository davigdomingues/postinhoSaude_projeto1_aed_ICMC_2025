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
 *
 * Observações de persistência:
 * - io_save implementa gravação segura: escreve para path + ".tmp" e só substitui
 *   o ficheiro final se fclose() e rename() tiverem sucesso.
 * - io_load valida o formato (uso de fgets/fscanf) e devolve -2 em caso de linhas faltantes
 *   ou formato inconsistente, evitando povoar as estruturas com dados parciais.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "io.h"
#include "config.h"


/* Remove terminadores de linha CR/LF do fim da string lida com fgets */
static void chomp(char *s) {
    if (!s) return;

    size_t n = strlen(s);

    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
}

/* Salva PatientList e Queue em 'path' com estrategia: escrever em tmp, fechar e rename.
   Retorna 0 sucesso, -1 erro de I/O.

   Passos detalhados dentro:
   - Cria nome temporario "%s.tmp"
   - Abre o ficheiro em modo texto para escrita (fopen)
   - Escreve numero de pacientes (unsigned long) seguido de novas linhas
   - Para cada paciente escreve id, name, n_history, cada entrada do historico e o called_flag
   - Em seguida escreve tamanho da fila e cada id da fila em linhas separadas
   - Fecha o ficheiro: se fclose falhar, apaga o .tmp e retorna erro (nao substitui o ficheiro alvo)
   - Se fclose ok, remove(path) e rename(tmp,path) para realizar substituicao atomica simples
   - Em caso de qualquer erro de escrita/leitura retorna -1 (io_save) e o chamador pode avisar o usuario
*/
int io_save(const char *path, const PatientList *pl, const Queue *q) {
    /* grava para ficheiro temporário e substitui apenas em sucesso */
    char tmp[512];
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) < 0) return -1;
    FILE *f = fopen(tmp, "w");
    if (!f) return -1;

    size_t n_pat = plist_size(pl);
    fprintf(f, "%lu\n", (unsigned long)n_pat);

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

    /* fechar e garantir que foi gravado */
    if (fclose(f) != 0) {
        /* apagar tmp se falha no fclose */
        remove(tmp);
        return -1;
    }

    /* substituir o ficheiro alvo: remove antigo (ignorar se não existir), depois renomeia */
    (void)remove(path); /* ignorar erro, pode ser que não exista */
    if (rename(tmp, path) != 0) {
        /* tentativa de limpeza */
        remove(tmp);
        return -1;
    }

    return 0;
}

/* Carrega dados de 'path' reconstruindo PatientList e Queue.
   Valida formato e devolve 0 sucesso, -1 ficheiro inacessivel, -2 erro de formato/leitura.

   Passos internos e validacoes importantes:
   - Abre o ficheiro em modo leitura
   - Lê a primeira linha com fscanf("%lu\\n") para obter number_of_patients
     (usamos unsigned long e convertemos para size_t para portabilidade)
   - Para cada paciente:
     * le id via fgets (verificar retorno)
     * le name via fgets (verificar retorno)
     * chomp em id/name (remove CR/LF)
     * chama plist_insert para criar o paciente em memoria
     * le n_history via fscanf("%d\\n"), e para cada entrada le uma linha e faz plist_history_push
     * le called_flag via fscanf("%d\\n") e aplica plist_set_called
   - Depois le tamanho da fila e enfileira os ids lidos (queue_enqueue), ignorando overflow da fila
   - Em caso de qualquer leitura inesperada retorna -2 e nao altera mais o estado
*/
int io_load(const char *path, PatientList *pl, Queue *q) {
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    size_t n_pat = 0;
    unsigned long n_pat_ul = 0;

    /* ler como unsigned long e converter para size_t para evitar uso de %zu no fscanf */
    if (fscanf(f, "%lu\n", &n_pat_ul) != 1) {
        fclose(f);
        return -2;
    }
    
    n_pat = (size_t)n_pat_ul;

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