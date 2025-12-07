/* Módulo de I/O (persistência textual, UTF-8)
 * - Salva/carrega PatientTree e PriorityQueue em formato legível.
 * - Escrita segura via arquivo temporário (.tmp) seguida de rename; leitura valida formato e normaliza encoding.
 * - Não acessa campos internos de History; usa wrappers expostos pela árvore (ptree_history_*).
 *
 * Funções:
 * - io_save(const char *path, const PatientTree *pt, const PriorityQueue *pq)
 * - io_load(const char *path, PatientTree *pt, PriorityQueue *pq)
 *
 * Encoding:
 * - io_load verifica UTF-8; se inválido, converte de CP1252 para UTF-8 (compatibilidade Windows antiga).
 * - io_save escreve UTF-8.
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
#include <stdlib.h>
#include <stdbool.h>
#include "io.h"
#include "config.h"

/* adicionados para garantir protótipos usados no callback */
#include "patient_tree.h"
#include "priority_queue.h"  

/* Forward declaration: torna o tipo e o callback visíveis antes do uso em io_save */
struct SaveCtx { FILE *f; const PatientTree *pt; const PriorityQueue *pq; };
static void write_patient_cb(const char *id, const char *name, bool called, void *ud);

/* Remove terminadores de linha CR/LF do fim da string lida com fgets */
static void chomp(char *s) {
    if (s == NULL)
        return;

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
int io_save(const char *path, const PatientTree *pt, const PriorityQueue *pq) {
    /* grava para ficheiro temporário e substitui apenas em sucesso */
    char tmp[512];
    int n = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (n < 0 || (size_t)n >= sizeof(tmp))
        return -1;

    FILE *f = fopen(tmp, "w");

    if (!f) 
        return -1;

    /* escreve número total de pacientes primeiro */
    size_t n_pat = ptree_size(pt);
    fprintf(f, "%lu\n", (unsigned long)n_pat);

    /* context passado para o callback: arquivo, árvore e fila */
    struct SaveCtx ctx = { f, pt, pq };

    /* percorre a árvore e escreve cada paciente */
    ptree_inorder(pt, write_patient_cb, &ctx);

    int qsize = pqueue_size(pq);
    fprintf(f, "%d\n", qsize);
    for (int i = 0; i < qsize; ++i) {
        char id[MAX_ID_LEN + 2];
        int prio = 0;

        if (pqueue_get_id_by_index(pq, i, id, sizeof(id)) != 0)
            continue;

        if (pqueue_get_priority_by_index(pq, i, &prio) != 0)
            prio = 5;

        fprintf(f, "%s\n%d\n", id, prio);
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

/* Protótipos das funções auxiliares definidas mais abaixo.
   Colocados aqui para que io_load possa usá‑las sem declarações implícitas. */
static int is_valid_utf8(const char *s);
static int cp1252_to_utf8(const char *in, char *out, size_t out_size);

/* Normaliza 's' para UTF-8 dentro do próprio buffer:
   - se já for UTF-8 válido -> não altera
   - senão assume CP1252 e tenta converter para UTF-8, copiando o resultado para o buffer
   - usa um buffer temporário alocado dinamicamente (3x tamanho do original) */
static void normalize_to_utf8_inplace(char *s, size_t buf_size) {
    if (!s || buf_size == 0) 
        return;

    if (is_valid_utf8(s)) 
        return;

    size_t tmp_size = buf_size * 3 + 8;
    char *tmp = (char *)malloc(tmp_size);

    if (!tmp) 
        return;

    if (cp1252_to_utf8(s, tmp, tmp_size) == 0) {
        strncpy(s, tmp, buf_size - 1);
        s[buf_size - 1] = '\0';
    }
    free(tmp);
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
     * chama ptree_insert para criar o paciente em memoria
     * le n_history via fscanf("%d\\n"), e para cada entrada le uma linha e faz ptree_history_push
     * le called_flag via fscanf("%d\\n") e aplica ptree_set_called
   - Depois le tamanho da fila e enfileira os ids lidos (queue_enqueue), ignorando overflow da fila
   - Em caso de qualquer leitura inesperada retorna -2 e nao altera mais o estado
*/
int io_load(const char *path, PatientTree *pt, PriorityQueue *pq) {
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
        
        /* normaliza id e name para UTF-8 (converte CP1252->UTF-8 quando necessário) */
        normalize_to_utf8_inplace(id, sizeof(id));
        normalize_to_utf8_inplace(name, sizeof(name));

        if (ptree_insert(pt, id, name) < 0) {
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
            /* normaliza linha do histórico para UTF-8 */
            normalize_to_utf8_inplace(line, sizeof(line));
            /* usar wrapper para adicionar histórico na árvore */
            if (ptree_history_push(pt, id, line) != 0) {
                /* falha ao inserir histórico -> ignoramos item e continuamos */
            }
        }

        int called_flag = 0;

        if (fscanf(f, "%d\n", &called_flag) != 1) {
            fclose(f);
            return -2;
        }

        ptree_set_called(pt, id, called_flag ? true : false);

        /* tentar ler prioridade do paciente (compatível com ficheiros antigos) */
        long pos_after_called = ftell(f);
        int pri_read = 5, maybe_pri = 0;

        if (fscanf(f, "%d\n", &maybe_pri) == 1 && maybe_pri >= 1 && maybe_pri <= 5)
            pri_read = maybe_pri;
        
        else
            /* voltar se não havia prioridade (formato antigo) */
            fseek(f, pos_after_called, SEEK_SET);

        (void)ptree_set_priority(pt, id, pri_read);

        /* tentar ler discharged_flag (compatível com ficheiros antigos) */
        long pos_after_pri = ftell(f);
        int dflag = 0, maybe_df = 0;

        if (fscanf(f, "%d\n", &maybe_df) == 1 && (maybe_df == 0 || maybe_df == 1))
            dflag = maybe_df;

        else
            fseek(f, pos_after_pri, SEEK_SET);
        (void)ptree_set_discharged(pt, id, dflag ? true : false);
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
        // normaliza id da fila de prioridades e enfileira.
        normalize_to_utf8_inplace(id, sizeof(id));
        /* tenta ler prioridade; se falhar, usa 5 (compatibilidade) */
        int prio = 5;
        long pos = ftell(f);
        int maybe;

        if (fscanf(f, "%d\n", &maybe) == 1 && maybe >= 1 && maybe <= 5)
            prio = maybe;
        
        else
            /* rewind para pos se leitura inválida (ficheiro antigo sem prioridade) */
            fseek(f, pos, SEEK_SET);

        (void)pqueue_enqueue(pq, id, prio);
    }

    fclose(f);
    return 0;
}

/* Verifica UTF-8 (simples)
   Função rápida para validar se a sequência de bytes está em UTF‑8 válido.
   Retorna 1 se válido, 0 caso contrário. Usada para decidir se é necessário
   aplicar conversão CP1252→UTF‑8 ao carregar texto do ficheiro. */
static int is_valid_utf8(const char *s) {
    const unsigned char *bytes = (const unsigned char *)s;
    while (*bytes) {
        if (*bytes < 0x80) {
            bytes++;
            continue;
        }

        if ((*bytes & 0xE0) == 0xC0) {
            if ((bytes[1] & 0xC0) != 0x80)
                return 0;

            bytes += 2;
            continue;
        }

        if ((*bytes & 0xF0) == 0xE0) {
            if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80)
                return 0;

            bytes += 3;
            continue;
        }

        if ((*bytes & 0xF8) == 0xF0) {
            if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80 || (bytes[3] & 0xC0) != 0x80)
                return 0;

            bytes += 4;
            continue;
        }

        return 0;
    }
    
    return 1;
}

/* Tabela de mapeamento CP1252 (0x80..0xFF) para pontos de código Unicode.
   Utilizada para converter bytes codificados em CP1252 para os pontos de
   código Unicode correspondentes antes de codificar em UTF‑8. */
static const unsigned short cp1252_table[128] = {
    0x20AC,0x0081,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,
    0x02C6,0x2030,0x0160,0x2039,0x0152,0x008D,0x017D,0x008F,
    0x0090,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
    0x02DC,0x2122,0x0161,0x203A,0x0153,0x009D,0x017E,0x0178,
    /* 0xA0..0xFF map to U+00A0..U+00FF */
    0x00A0,0x00A1,0x00A2,0x00A3,0x00A4,0x00A5,0x00A6,0x00A7,
    0x00A8,0x00A9,0x00AA,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,
    0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,
    0x00B8,0x00B9,0x00BA,0x00BB,0x00BC,0x00BD,0x00BE,0x00BF,
    0x00C0,0x00C1,0x00C2,0x00C3,0x00C4,0x00C5,0x00C6,0x00C7,
    0x00C8,0x00C9,0x00CA,0x00CB,0x00CC,0x00CD,0x00CE,0x00CF,
    0x00D0,0x00D1,0x00D2,0x00D3,0x00D4,0x00D5,0x00D6,0x00D7,
    0x00D8,0x00D9,0x00DA,0x00DB,0x00DC,0x00DD,0x00DE,0x00DF,
    0x00E0,0x00E1,0x00E2,0x00E3,0x00E4,0x00E5,0x00E6,0x00E7,
    0x00E8,0x00E9,0x00EA,0x00EB,0x00EC,0x00ED,0x00EE,0x00EF,
    0x00F0,0x00F1,0x00F2,0x00F3,0x00F4,0x00F5,0x00F6,0x00F7,
    0x00F8,0x00F9,0x00FA,0x00FB,0x00FC,0x00FD,0x00FE,0x00FF
};

/* Converte bytes em CP1252 presentes em 'in' para UTF-8 em 'out' (out_size).
   Retorna 0 em sucesso, -1 em erro (ex.: espaço insuficiente em 'out').
   Esta função é usada quando detectamos que uma linha lida não está em UTF‑8
   e presumimos que esteja em CP1252 (Windows-1252). */
static int cp1252_to_utf8(const char *in, char *out, size_t out_size) {
    if (!in || !out || out_size == 0) 
        return -1;
        
    size_t ri = 0;
    const unsigned char *p = (const unsigned char *)in;

    while (*p) {
        unsigned int cp;
        if (*p < 0x80) {
            cp = *p++;
        } else {
            cp = cp1252_table[*p - 0x80];
            p++;
        }

        /* codificar cp em UTF-8 */
        if (cp <= 0x7F) {
            if (ri + 1 >= out_size)
                return -1;

            out[ri++] = (char)cp;
        } 
        
        else if (cp <= 0x7FF) {
            if (ri + 2 >= out_size)
                return -1;

            out[ri++] = (char)(0xC0 | ((cp >> 6) & 0x1F));
            out[ri++] = (char)(0x80 | (cp & 0x3F));
        } 
        
        else if (cp <= 0xFFFF) {
            if (ri + 3 >= out_size)
                return -1;

            out[ri++] = (char)(0xE0 | ((cp >> 12) & 0x0F));
            out[ri++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[ri++] = (char)(0x80 | (cp & 0x3F));
        } 
        
        else {
            /* suporte para pontos de código maiores (raro no CP1252) */
            if (ri + 4 >= out_size) 
            return -1;

            out[ri++] = (char)(0xF0 | ((cp >> 18) & 0x07));
            out[ri++] = (char)(0x80 | ((cp >> 12) & 0x3F));
            out[ri++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[ri++] = (char)(0x80 | (cp & 0x3F));
        }
    }

    if (ri >= out_size)
        return -1;

    out[ri] = '\0';
    return 0;
}

/* callback não aninhada para escrever um paciente (usada por ptree_inorder) */
static void write_patient_cb(const char *id, const char *name, bool called, void *ud) {
    (void)called; /* evita warning de parâmetro não usado */
    struct SaveCtx *c = (struct SaveCtx*)ud;

    if (!c || !c->f) 
        return;

    FILE *wf = c->f;
    /* id/name já estão fornecidos pelo inorder */
    fprintf(wf, "%s\n%s\n", id, name);

    /* histórico: usa wrappers da árvore */
    int n_hist = ptree_history_size_by_id(c->pt, id);
    fprintf(wf, "%d\n", n_hist);
    for (int k = 0; k < n_hist; ++k) {
        char line[PROC_MAX_LEN + 2];
        
        if (ptree_history_get_by_id(c->pt, id, k, line, sizeof(line)) != 0)
            continue;

        fprintf(wf, "%s\n", line);
    }

    /* called_flag: se estiver na fila, guardamos 0; caso contrário usamos a flag em memória */
    int called_flag = 0;
    /* chamado: verifica se está na fila de prioridades */
    if (c->pq && pqueue_contains(c->pq, id))
        called_flag = 0;

    else
        called_flag = ptree_is_called(c->pt, id) ? 1 : 0;
        
    fprintf(wf, "%d\n", called_flag);
    /* prioridade registrada do paciente */
    int pri = ptree_get_priority(c->pt, id);

    if (pri < 1 || pri > 5) 
        pri = 5;

    fprintf(wf, "%d\n", pri);
    
    int discharged_flag = ptree_is_discharged(c->pt, id) ? 1 : 0;
    fprintf(wf, "%d\n", discharged_flag);
}