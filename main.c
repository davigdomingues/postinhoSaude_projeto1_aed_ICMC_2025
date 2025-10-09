/* 
Este código contém a função main() da aplicação "postinho de saúde".
Objetivo geral:
- Gerir uma lista de pacientes (pl) e uma fila de espera (q).
- Fornecer um menu simples em linha de comando para registrar pacientes,
  registrar óbito (com restrições), adicionar/desfazer procedimentos no histórico,
  chamar o próximo, mostrar a fila, exibir histórico e salvar os dados ao sair.

Inclusões e módulos:
- config.h: constantes de configuração (tamanhos máximos, capacidade da fila).
- patient_list.h: interface para manipular a lista de pacientes (inserir, buscar, obter, liberar).
- queue.h: interface para fila de espera (inicializar, enfileirar, desenfileirar, verificar existência/cheia, liberar).
- history.h: interface para o histórico de procedimentos por paciente (push, pop, verificar cheio).
- io.h: funções para salvar/carregar dados persistentes (io_save, io_load).
- util.h: utilitários de I/O (read_line, util_printf, format_timestamp).

Estruturas usadas em runtime (alocadas dinamicamente):
- PatientList *pl;  // ponteiro para a lista de pacientes alocada por plist_create()
- Queue *q;         // ponteiro para a fila de espera alocada por queue_create()
- clear_screen.h: utilidades de UI (limpar tela, mensagens temporizadas).

Fluxo principal (main):
1. Inicialização:
   - plist_create(): prepara a estrutura da lista de pacientes.
   - queue_create(WAIT_CAP): cria a fila com capacidade definida em config.h.

2. Loop do menu:
   - Exibe opções numeradas de 1 a 8.
   - Lê uma linha com fgets() para evitar estouro de buffer e converte para inteiro com atoi().
   - Cada opção chama funções dos módulos correspondentes e realiza verificações:

     1) Registrar paciente:
        - Lê ID e verifica se já existe (plist_find_index).
        - Se não existir, lê nome e insere (plist_insert).
        - Se o paciente já existir, ele pode ser reinscrito na fila, desde que seja informado o ID correto, a primeira vista.
        - Tenta enfileirar o paciente (queue_enqueue) com checagens: fila cheia (queue_is_full) ou paciente já na fila (queue_contains).

     2) Dar alta:
        - Lê ID e remove da fila (queue_remove). Retorno 0 => sucesso.
        - Remove paciente da lista (plist_remove).

     3) Adicionar procedimento ao histórico:
        - Lê ID e busca paciente (plist_get). Se não encontrado, avisa.
        - Verifica se histórico cheio (history_is_full).
        - Lê descrição do procedimento e faz history_push(&p->hist, proc).

     4) Desfazer último procedimento:
        - Lê ID, obtém paciente e faz history_pop(&p->hist, out, sizeof(out)).

     5) Chamar próximo:
        - queue_dequeue(&q, id, sizeof(id)) remove o próximo da fila e coloca o ID em 'id'.
        - Usa plist_get para tentar recuperar o nome do paciente (pode ser NULL se o cadastro não existir).

     6) Mostrar fila:
        - lista a fila resolvendo nomes via PatientList (sem usar função de impressão do TAD).
        - Se a fila estiver vazia, avisa o usuário.

     7) Mostrar histórico:
        - Obtém paciente e imprime p->hist.top + 1 itens.
        - Se o histórico estiver vazio, avisa o usuário.

     8) Sair:
        - Salva dados em arquivo com io_save(DATA_FILE, &pl, &q) e sai do loop.
        
Tratamento de erros e convenções:
- Muitas funções retornam 0 em caso de sucesso e valor != 0 em erro — o main assume essa convenção.
- fgets() é usado para ler entrada do utilizador e evitar overflow; atoi() para converter a opção.
- Variáveis temporárias (id, name, proc) usam tamanhos definidos em config.h (MAX_ID_LEN, MAX_NAME_LEN, PROC_MAX_LEN).
- Mensagens informativas são exibidas ao usuário em cada caminho de execução.

Limpeza:
- Antes de terminar, a aplicação chama queue_destroy(q) e plist_destroy(pl) para libertar recursos dinâmicos alocados pelos módulos.

Observações de integração:
- A maior parte da lógica "pesada" (pesquisa, memória, histórico) está em módulos separados (patient_list, queue, history, io, util).
- Persistência: main.c chama io_load(DATA_FILE, ...) no arranque e io_save(DATA_FILE, ...) ao sair.
  * io_save escreve para um ficheiro temporário e só renomeia para DATA_FILE em sucesso (comportamento atômico simples).
  * main.c evita sobrescrever DATA_FILE quando a carga inicial falha e não houve alterações na sessão.
- UI: main.c usa message_and_clear/clear_screen e faz pausa explícita (read_line) após mostrar dados carregados para permitir leitura pelo usuário.
*/

#define _XOPEN_SOURCE 600
#include <stdio.h> // para printf() e FILE
#include <stdlib.h> // para atoi() e alocação
#include <ctype.h> // para isdigit() e ispace()
#include <string.h> // para manipular strings mais facilmente
#include "config.h" // header com constantes de configuração
#include "patient_list.h" // header da lista de pacientes
#include "queue.h" // header da fila
#include "history.h" // header do histórico
#include "io.h" // header para salvar/carregar
#include "util.h" // header com read_line()
#include "clear_screen.h" // header para limpar a tela

/* locale / widechar suporte para nomes acentuados */
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#if defined(_WIN32)
#include <windows.h>
#endif

/* Helpers locais para melhorar legibilidade */

/* Imprime o menu principal no terminal (sem lógica de leitura)
   Esta função só apresenta as opções disponíveis para o usuário */
static void show_menu(void) {
    printf("\nMenu:\n");
    printf("1. Registrar paciente\n");
    printf("2. Registrar obito de paciente\n");
    printf("3. Adicionar procedimento ao historico medico do paciente\n");
    printf("4. Desfazer procedimento do historico medico do paciente\n");
    printf("5. Chamar paciente para atendimento\n");
    printf("6. Mostrar fila de espera\n");
    printf("7. Mostrar historico medico do paciente\n");
    printf("8. Sair\n");
    printf("Escolha: ");
}

/* Mostra informações carregadas do ficheiro de dados:
   - lista de pacientes (resumo)
   - históricos por paciente
   - fila de espera com nomes resolvidos via PatientList
   Em caso de erro no carregamento, informa o usuário e inicializa vazio */
static void show_archive_data(PatientList *pl, Queue *q, int load_r) {
    /* Tenta carregar dados persistidos (se existir) */
    if (load_r == 0) {
        /* Mostra resumidamente o estado carregado para o utilizador antes de limpar */
        printf("Dados carregados a partir de %s.\n\n", DATA_FILE);

        if (plist_size(pl) > 0) {
            plist_print(pl);

            /* Mostra o histórico detalhado de cada paciente */
            for (size_t pi = 0; pi < plist_size(pl); ++pi) {
                char pid[MAX_ID_LEN + 1];
                char pname[MAX_NAME_LEN + 1];
                if (plist_get_id_by_index(pl, pi, pid, sizeof(pid)) != 0) continue;
                if (plist_get_name_by_index(pl, pi, pname, sizeof(pname)) != 0) strncpy(pname, "(desconhecido)", sizeof(pname));

                int hsz = plist_history_size_by_index(pl, pi);
                util_printf("\nHistorico de %s (ID %s): %d item(ns)\n", pname, pid, hsz);
                for (int hi = 0; hi < hsz; ++hi) {
                    char hline[PROC_MAX_LEN + 1];
                    if (plist_history_get_by_index(pl, pi, hi, hline, sizeof(hline)) == 0)
                        util_printf("  %d) %s\n", hi + 1, hline);
                }
            }
        } 
        
        else
            printf("Nenhum paciente registrado.\n");

        printf("\n");

        if (queue_size(q) > 0) {
            printf("Fila de espera:\n");
            int qsz = queue_size(q);
            for (int qi = 0; qi < qsz; ++qi) {
                char qid[MAX_ID_LEN + 1];
                char qname[MAX_NAME_LEN + 1];
                if (queue_get_id_by_index(q, qi, qid, sizeof(qid)) != 0) continue;
                if (plist_get_name_by_id(pl, qid, qname, sizeof(qname)) != 0)
                    strncpy(qname, "(desconhecido)", sizeof(qname));
                util_printf("%d: %s - %s\n", qi + 1, qid, qname);
            }
        } 
        
        else
            printf("\nFila de espera vazia.\n");

        /* Em vez de limpar automaticamente, aguarda que o utilizador pressione Enter
           para garantir que as impressões permaneçam visíveis. */
        {
            char __tmp_wait[8];
            printf("\nPressione Enter para continuar...");
            fflush(stdout);
            read_line(__tmp_wait, sizeof(__tmp_wait));
            clear_screen();
        }
    } else if (load_r == -1) {
        /* arquivo inexistente: inicialização com estruturas vazias (normal em primeira execução) */
        printf("Nenhum arquivo de dados encontrado.\n");
        message_and_clear("Iniciando com banco vazio.", MSG_WAIT_SHORT);
    } else {
        printf("Erro ao carregar dados (formato/IO).\n");
        message_and_clear("Iniciando com banco vazio.", MSG_WAIT_SHORT);
    }

    /* Mensagem de boas-vindas simples (sem limpar novamente de forma imediata) */
    message_and_clear("Bem-vindo ao PostinhoSUS - Sistema de Gestao (Projeto AED, ICMC 2025).", MSG_WAIT_SHORT);
}

/* Funcao principal do programa simplificada:
   - inicializa estruturas (PatientList, Queue)
   - tenta carregar dados persistidos (io_load)
   - executa loop de menu interpretando as opcoes 1..8
   - delega operacoes aos modulos (plist_*, queue_*, history_*, io_*)
   - salva condicionalmente e libera recursos antes de terminar
*/
int main(){
    /* configuração local para suporte à dados multibyte (acentuados) */
    util_setup_locale();
#if 0
    /* antigo bloco mantido aqui apenas como referência (removido) */
    /* define locale a partir do ambiente; preferir UTF-8 quando disponível */
    setlocale(LC_ALL, "");
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    system("chcp 65001 > nul");
#else
    if (!getenv("LANG") && !getenv("LC_ALL")) {
        setenv("LC_ALL", "en_US.UTF-8", 0);
        setlocale(LC_ALL, "");
    }
#endif
#endif

    PatientList *pl = plist_create();
    Queue       *q  = queue_create(WAIT_CAP);
    
    if (!pl || !q) {
        fprintf(stderr, "Erro de inicializacao.\n");
        return 1;
    }

    int load_r = io_load(DATA_FILE, pl, q);
    show_archive_data(pl, q, load_r);

    int opc = 0;
    char buf[256];

    for (;;) {
        show_menu();

        if (!fgets(buf, sizeof(buf), stdin))
            break;

        opc = atoi(buf);

        clear_screen(); // limpa a tela após a escolha

        if (opc == 1) {
            char id[MAX_ID_LEN + 1], name[MAX_NAME_LEN + 1];
            int reinInserted = 0; /* flag: se 1, pula cadastro pois tratou reinsercao/aviso */
            /* Lê ID manualmente, aceita apenas dígitos e evita duplicata.
               Faz trim de espaços, usa strnlen e valida cada caractere com isdigit. */
            for (;;) {
                printf("ID (somente digitos): ");
                read_line(id, sizeof(id));

                /* rejeita imediatamente se a entrada foi maior que o buffer */
                if (read_line_truncated()) {
                    message_and_clear("ID muito longo. Tente novamente.", MSG_WAIT_SHORT);
                    continue;
                }

                /* trim: remover espaços iniciais e finais */
                size_t start = 0;
                while (id[start] && isspace((unsigned char)id[start]))
                    ++start;

                size_t end = strnlen(id, MAX_ID_LEN + 1);
                while (end > start && isspace((unsigned char)id[end - 1]))
                    --end;

                if (start != 0 || end != strlen(id)) {
                    /* compacta a string */
                    size_t len = end - start;
                    if (len > 0) memmove(id, id + start, len);
                    id[len] = '\0';
                }

                size_t len = strnlen(id, MAX_ID_LEN + 1);
                if (len == 0) {
                    message_and_clear("ID vazio. Informe novamente.", MSG_WAIT_SHORT);
                    continue;
                }

                if (len > MAX_ID_LEN) {
                    message_and_clear("ID muito longo. Tente novamente.", MSG_WAIT_SHORT);
                    continue;
                }

                int all_digits = 1;
                for (size_t i = 0; i < len; ++i)
                    if (!isdigit((unsigned char)id[i])) { all_digits = 0; break; }

                if (!all_digits) {
                    message_and_clear("ID invalido. Use apenas caracteres numericos (0-9).", MSG_WAIT_SHORT);
                    continue;
                }

                /* Se o ID já está cadastrado, oferecer reinsercao na fila:
                   - se já estiver na fila, avisa e retornar ao menu
                   - se fila cheia, avisa e retornar ao menu
                   - senao, enfileira e informa "paciente reinserido na fila!" */
                if (plist_find_index(pl, id) >= 0) {
                    if (queue_contains(q, id)) {
                        message_and_clear("Paciente ja esta na fila. Retornando ao menu...", MSG_WAIT_SHORT);
                        reinInserted = 1; /* tratou a situacao, não cadastrar */
                        break;
                    } else if (queue_is_full(q)) {
                        message_and_clear("Fila cheia. Nao foi possivel inserir.", MSG_WAIT_SHORT);
                        reinInserted = 1; /* não cadastrar */
                        break;
                    } else {
                        queue_enqueue(q, id);
                        /* paciente voltou para a fila -> marcar como não chamado */
                        (void)plist_set_called(pl, id, false);
                        message_and_clear("Paciente reinserido na fila!", MSG_WAIT_SHORT);
                        reinInserted = 1; /* já reinserido, pular cadastro */
                        break;
                    }
                }

                break;
            }

            /* Se já tratamos reinsercao/aviso, retorna ao menu sem tentar cadastrar nome */
            if (reinInserted) {
                continue;
            }

            /* Lê e aceita nome arbitrário (qualquer string não vazia).
               Observação: aceita acentos e outros caracteres sem validação por caractere.
               Rejeita nomes truncados ou vazios. */
            for (;;) {
                printf("Nome: "); read_line(name, sizeof(name));

                /* entrada maior que o buffer é truncada -> pedir novamente */
                if (read_line_truncated()) {
                    message_and_clear("Nome muito longo. Tente novamente.", MSG_WAIT_SHORT);
                    continue;
                }

                /* nome vazio não é aceito */
                if (name[0] == '\0') {
                    message_and_clear("Nome vazio. Informe novamente.", MSG_WAIT_SHORT);
                    continue;
                }

                break;
            }

            int r = plist_insert(pl, id, name);
            if (r == 0)
                printf("Paciente cadastrado.\n");
            else {
                printf("Falha ao cadastrar.\n");
                message_and_clear("Falha ao cadastrar. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (queue_is_full(q))
                printf("Fila cheia. Nao foi possivel inserir.\n");
            else if (queue_contains(q, id))
                printf("Paciente ja esta na fila de espera.\n");
            else {
                queue_enqueue(q, id);
                printf("Paciente inserido na fila.\n");
            }

            message_and_clear("Operacao concluida. Retornando ao menu...", MSG_WAIT_SHORT);

        } else if (opc == 2) { // Para o trabalho, foi escolhido um cenário ideal em que o paciente só morreria, caso não estivesse na fila.
            char id[MAX_ID_LEN + 1];
            printf("ID do obito: "); read_line(id, sizeof(id));

            if (plist_find_index(pl, id) < 0) {
                message_and_clear("Paciente nao encontrado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (queue_contains(q, id)) {
                printf("Obito proibido.\n");
                message_and_clear("Paciente ainda esta na fila. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (!plist_is_called(pl, id)) {
                printf("Obito proibido.\n");
                message_and_clear("Paciente nao foi chamado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (plist_remove(pl, id) == 0)
                printf("Obito registrado e dados removidos (LGPD).\n");
            else
                printf("Falha ao remover registro do paciente.\n");

            message_and_clear("Operacao concluida. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 3) {
            char id[MAX_ID_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));

            if (plist_find_index(pl, id) < 0) {
                message_and_clear("Paciente nao encontrado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (plist_history_is_full(pl, id)) {
                message_and_clear("Historico cheio. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            /* lê descrição com validações e prefixa timestamp via util::format_timestamp */
            char proc[PROC_MAX_LEN + 1];
            printf("Procedimento (ate %d chars): ", PROC_MAX_LEN);
            read_line(proc, sizeof(proc));

            if (read_line_truncated()) {
                message_and_clear("Descricao muito longa. Tente novamente.", MSG_WAIT_SHORT);
                continue;
            }

            if (proc[0] == '\0') {
                message_and_clear("Descricao vazia. Tente novamente.", MSG_WAIT_SHORT);
                continue;
            }

            /* obter timestamp formatado (se disponível) */
            char timestr[32] = {0};
            if (format_timestamp(timestr, sizeof(timestr)) != 0)
                timestr[0] = '\0';

            /* montar item com timestamp seguro e truncado para PROC_MAX_LEN */
            char item[PROC_MAX_LEN + 1];
            item[0] = '\0';
            if (timestr[0] != '\0') {
                /* escreve somente o prefixo "[timestr] " e depois concatena o proc
                   limitando a cópia ao espaço restante para evitar warnings do compilador */
                int pref = snprintf(item, sizeof(item), "[%s] ", timestr);
                if (pref < 0) pref = 0;
                size_t used = (size_t)pref;
                if (used >= sizeof(item)) {
                    /* já cheio; garante terminação */
                    item[sizeof(item) - 1] = '\0';
                } else {
                    size_t avail = sizeof(item) - used - 1; /* espaço restante para chars + '\0' */
                    /* strncat usa o espaço disponível; garante terminação */
                    strncat(item, proc, avail);
                }
            } else {
                /* sem timestamp: copia procedure com segurança */
                strncpy(item, proc, sizeof(item) - 1);
                item[sizeof(item) - 1] = '\0';
            }

            if (plist_history_push(pl, id, item) == 0)
                printf("Procedimento adicionado.\n");
            else
                printf("Falha ao adicionar procedimento (historico cheio ou erro).\n");

            message_and_clear("Operacao concluida. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 4) {
            char id[MAX_ID_LEN + 1], out[PROC_MAX_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));

            if (plist_find_index(pl, id) < 0) {
                message_and_clear("Paciente nao encontrado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            /* Pop desfaz o ultimo procedimento (LIFO) */
            if (plist_history_pop(pl, id, out, sizeof(out)) == 0)
                printf("Procedimento desfeito (ultimo): %s\n", out);
            else
                message_and_clear("Nao ha procedimento a desfazer. Retornando ao menu...", MSG_WAIT_SHORT);

            message_and_clear("Operacao concluida. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 5) {
            char id[MAX_ID_LEN + 1];
            printf("Chamando proximo...\n");

            if (queue_dequeue(q, id, sizeof(id)) == 0) {
                char name[MAX_NAME_LEN + 1];
                if (plist_get_name_by_id(pl, id, name, sizeof(name)) == 0) {
                    plist_set_called(pl, id, true);
                    util_printf("Chamando: ID %s | Nome: %s\n", id, name);
                } else
                    printf("Chamando: ID %s | Nome: (desconhecido)\n", id);
            } else
                printf("Fila vazia.\n");

            message_and_clear("Operacao concluida. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 6) {
            /* Imprimir fila com nomes associados aos IDs */
            int qsize = queue_size(q);
            if (qsize == 0) {
                message_and_clear("Fila vazia. Retornando ao menu...", MSG_WAIT_SHORT);
            } else {
                printf("Fila de espera (total = %d):\n", qsize);
                for (int i = 0; i < qsize; ++i) {
                    char id[MAX_ID_LEN + 1];
                    char name[MAX_NAME_LEN + 1];
                    if (queue_get_id_by_index(q, i, id, sizeof(id)) != 0) continue;
                    if (plist_get_name_by_id(pl, id, name, sizeof(name)) != 0)
                        strncpy(name, "(desconhecido)", sizeof(name));
                    printf("%d: %s - %s\n", i + 1, id, name);
                }
                message_and_clear("Retornando ao menu...", MSG_WAIT_SHORT);
            }
        } else if (opc == 7) {
            char id[MAX_ID_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));

            if (plist_find_index(pl, id) < 0) {
                printf("Paciente nao encontrado.\n");
                message_and_clear("Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            int n = plist_history_size_by_id(pl, id);
            char name[MAX_NAME_LEN + 1];
            plist_get_name_by_id(pl, id, name, sizeof(name));

            util_printf("Historico de %s (ID %s): %d item(ns)\n", name, id, n);

            for (int i = 0; i < n; ++i) {
                char item[PROC_MAX_LEN + 1];
                if (plist_history_get_by_id(pl, id, i, item, sizeof(item)) == 0)
                    util_printf("%d) %s\n", i + 1, item);
            }

            message_and_clear("Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 8) {
            /* Só salva se houver dados carregados ou se existirem entradas geradas */
            size_t n_pat = plist_size(pl);
            int qsize = queue_size(q);
            if (n_pat > 0 || qsize > 0 || load_r == 0) {
                if (io_save(DATA_FILE, pl, q) == 0)
                    printf("Dados salvos em %s. Ate breve.\n", DATA_FILE);
                else
                    printf("Erro ao salvar dados.\n");
            } else {
                printf("Nenhum dado para salvar. Arquivo nao foi alterado.\n");
            }
 
             break;
        } else {
            printf("Opcao invalida.\n");
            message_and_clear("Opcao invalida. Retornando ao menu...", MSG_WAIT_SHORT);
        }
    }

    queue_destroy(q);
    plist_destroy(pl);

    return 0;
}