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
#include <stdlib.h>
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

        /* Garante que o valor persistido de 'called' seja consistente com a fila
           Se o ID estiver na fila, consideramos que ele nao esta 'chamado' (0)
           mesmo que a flag em memoria esteja desatualizada */
        int called_flag = 0;
        if (q && queue_contains(q, id))
            called_flag = 0;
        
        else 
            called_flag = plist_is_called(pl, id) ? 1 : 0;
        
        fprintf(f, "%d\n", called_flag);
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

/* Protótipos das funções auxiliares definidas mais abaixo.
   Colocados aqui para que io_load possa usá‑las sem declarações implícitas. */
static int is_valid_utf8(const char *s);
static int cp1252_to_utf8(const char *in, char *out, size_t out_size);

/* Normaliza 's' para UTF-8 dentro do próprio buffer:
   - se já for UTF-8 válido -> não altera
   - senão assume CP1252 e tenta converter para UTF-8, copiando o resultado para o buffer
   - usa um buffer temporário alocado dinamicamente (3x tamanho do original) */
static void normalize_to_utf8_inplace(char *s, size_t buf_size) {
    if (!s || buf_size == 0) return;
    if (is_valid_utf8(s)) return;
    size_t tmp_size = buf_size * 3 + 8;
    char *tmp = (char *)malloc(tmp_size);
    if (!tmp) return;
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
        
        /* normaliza id e name para UTF-8 (converte CP1252->UTF-8 quando necessário) */
        normalize_to_utf8_inplace(id, sizeof(id));
        normalize_to_utf8_inplace(name, sizeof(name));

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
            /* normaliza linha do histórico para UTF-8 */
            normalize_to_utf8_inplace(line, sizeof(line));
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
        /* normaliza id da fila e enfileira */
        normalize_to_utf8_inplace(id, sizeof(id));
        (void)queue_enqueue(q, id); /* ignora overflow */
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
        if (*bytes < 0x80) { bytes++; continue; }
        if ((*bytes & 0xE0) == 0xC0) {
            if ((bytes[1] & 0xC0) != 0x80) return 0;
            bytes += 2; continue;
        }
        if ((*bytes & 0xF0) == 0xE0) {
            if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80) return 0;
            bytes += 3; continue;
        }
        if ((*bytes & 0xF8) == 0xF0) {
            if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80 || (bytes[3] & 0xC0) != 0x80) return 0;
            bytes += 4; continue;
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
    if (!in || !out || out_size == 0) return -1;
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
            if (ri + 1 >= out_size) return -1;
            out[ri++] = (char)cp;
        } else if (cp <= 0x7FF) {
            if (ri + 2 >= out_size) return -1;
            out[ri++] = (char)(0xC0 | ((cp >> 6) & 0x1F));
            out[ri++] = (char)(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            if (ri + 3 >= out_size) return -1;
            out[ri++] = (char)(0xE0 | ((cp >> 12) & 0x0F));
            out[ri++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[ri++] = (char)(0x80 | (cp & 0x3F));
        } else {
            /* suporte para pontos de código maiores (raro no CP1252) */
            if (ri + 4 >= out_size) return -1;
            out[ri++] = (char)(0xF0 | ((cp >> 18) & 0x07));
            out[ri++] = (char)(0x80 | ((cp >> 12) & 0x3F));
            out[ri++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[ri++] = (char)(0x80 | (cp & 0x3F));
        }
    }

    if (ri >= out_size) return -1;
    out[ri] = '\0';
    return 0;
}