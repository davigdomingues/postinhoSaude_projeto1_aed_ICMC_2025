/* Explicação detalhada de main.c

Este código contém a função main() da aplicação "postinho de saúde".
Objetivo geral:
- Gerir uma lista de pacientes (pl) e uma fila de espera (q).
- Fornecer um menu simples em linha de comando para registrar pacientes,
  dar alta, adicionar/desfazer procedimentos no histórico, chamar o próximo,
  mostrar a fila, exibir histórico e salvar os dados ao sair.

Inclusões e módulos:
- config.h: constantes de configuração (tamanhos máximos, capacidade da fila).
- patient_list.h: interface para manipular a lista de pacientes (inserir, buscar, obter, liberar).
- queue.h: interface para fila de espera (inicializar, enfileirar, desenfileirar, remover, verificar existência/cheia, imprimir, liberar).
- history.h: interface para o histórico de procedimentos por paciente (push, pop, verificar cheio).
- io.h: funções para salvar/carregar dados persistentes (io_save, possivelmente io_load).
- util.h: utilitários de I/O, por exemplo read_line() para ler linhas com segurança.

Estruturas globais:
- PatientList pl;  // armazena pacientes (id, nome, histórico, ...)
- Queue q;         // fila de espera com capacidade WAIT_CAP

Fluxo principal (main):
1. Inicialização:
   - plist_init(&pl): prepara a estrutura da lista de pacientes.
   - queue_init(&q, WAIT_CAP): cria a fila com capacidade definida em config.h.

2. Loop do menu:
   - Exibe opções numeradas de 1 a 8.
   - Lê uma linha com fgets() para evitar estouro de buffer e converte para inteiro com atoi().
   - Cada opção chama funções dos módulos correspondentes e realiza verificações:

     1) Registrar paciente:
        - Lê ID e verifica se já existe (plist_find_index).
        - Se não existir, lê nome e insere (plist_insert).
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
        - queue_print(&q) imprime os elementos da fila em ordem.
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
- Antes de terminar, a aplicação chama queue_free(&q) e plist_free(&pl) para libertar recursos dinâmicos alocados pelos módulos.

Observações de integração:
- A maior parte da lógica "pesada" (inserção/remoção, memória) está em módulos separados (patient_list, queue, history, io). main.c coordena e valida entradas.
- Para depuração/portabilidade: considere verificar retornos de funções de I/O mais detalhadamente e normalizar tratamentos de strings (trim), mas a estrutura atual é adequada para um protótipo educativo.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> /* para isalpha() */
#include "config.h" // header com constantes de configuração
#include "patient_list.h" // header da lista de pacientes
#include "queue.h" // header da fila
#include "history.h" // header do histórico
#include "io.h" // header para salvar/carregar
#include "util.h" // header com read_line()
#include "clear_screen.h" // header para limpar a tela

/* Helpers locais para melhorar legibilidade */
static void show_menu(void) {
    printf("\nMenu:\n");
    printf("1. Registrar paciente\n");
    printf("2. Registrar óbito de paciente\n");
    printf("3. Adicionar procedimento ao histórico\n");
    printf("4. Desfazer último procedimento\n");
    printf("5. Chamar paciente para atendimento\n");
    printf("6. Mostrar fila de espera\n");
    printf("7. Mostrar histórico do paciente\n");
    printf("8. Sair\n");
    printf("Escolha: ");
}

int main(){
    PatientList *pl = plist_create();
    Queue       *q  = queue_create(WAIT_CAP);
    if (!pl || !q) {
        fprintf(stderr, "Erro de inicialização.\n");
        return 1;
    }

    clear_screen();
    message_and_clear("Bem-vindo ao PostinhoSUS — Sistema de Gestão (Projeto AED, ICMC 2025).", MSG_WAIT_MEDIUM);

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
            /* Ler ID manualmente, aceitar apenas dígitos e evitar duplicata.
               Faz trim de espaços, usa strnlen e valida cada caractere com isdigit. */
            for (;;) {
                printf("ID (somente dígitos): ");
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
                    message_and_clear("ID inválido. Use apenas caracteres numéricos (0-9).", MSG_WAIT_SHORT);
                    continue;
                }

                if (plist_find_index(pl, id) >= 0) {
                    message_and_clear("ID já registrado. Informe outro ID.", MSG_WAIT_SHORT);
                    continue;
                }

                break;
            }

            /* Lê e valida nome: apenas letras e espaços (até aceitar) */
            for (;;) {
                printf("Nome: "); read_line(name, sizeof(name));

                if (name[0] == '\0') {
                    message_and_clear("Nome vazio. Informe novamente.", MSG_WAIT_SHORT);
                    continue;
                }

                int valid = 1;
                for (const unsigned char *p = (const unsigned char *)name; *p; ++p)
                    if (!isalpha(*p) && *p != ' ') { valid = 0; break; }

                if (valid) break;
                message_and_clear("Nome inválido. Use apenas letras e espaços.", MSG_WAIT_SHORT);
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
                printf("Fila cheia. Não foi possível inserir.\n");
            else if (queue_contains(q, id))
                printf("Paciente já está na fila de espera.\n");
            else {
                queue_enqueue(q, id);
                printf("Paciente inserido na fila.\n");
            }

            message_and_clear("Operação concluída. Retornando ao menu...", MSG_WAIT_SHORT);

        } else if (opc == 2) {
            char id[MAX_ID_LEN + 1];
            printf("ID do óbito: "); read_line(id, sizeof(id));

            if (plist_find_index(pl, id) < 0) {
                message_and_clear("Paciente não encontrado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (queue_contains(q, id)) {
                printf("Óbito proibido.\n");
                message_and_clear("Paciente ainda está na fila. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (!plist_is_called(pl, id)) {
                printf("Óbito proibido.\n");
                message_and_clear("Paciente não foi chamado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (plist_remove(pl, id) == 0)
                printf("Óbito registrado e dados removidos (LGPD).\n");
            else
                printf("Falha ao remover registro do paciente.\n");

            message_and_clear("Operação concluída. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 3) {
            char id[MAX_ID_LEN + 1], proc[PROC_MAX_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));

            if (plist_find_index(pl, id) < 0) {
                message_and_clear("Paciente não encontrado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (plist_history_is_full(pl, id)) {
                message_and_clear("Histórico cheio. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            printf("Procedimento (até %d chars): ", PROC_MAX_LEN);
            read_line(proc, sizeof(proc));

            if (plist_history_push(pl, id, proc) == 0)
                printf("Procedimento adicionado.\n");
            else
                printf("Falha ao adicionar procedimento.\n");

            message_and_clear("Operação concluída. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 4) {
            char id[MAX_ID_LEN + 1], out[PROC_MAX_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));

            if (plist_find_index(pl, id) < 0) {
                message_and_clear("Paciente não encontrado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (plist_history_pop(pl, id, out, sizeof(out)) == 0)
                printf("Procedimento desfeito: %s\n", out);
            else
                printf("Não há procedimento a desfazer.\n");

            message_and_clear("Operação concluída. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 5) {
            char id[MAX_ID_LEN + 1];
            printf("Chamando próximo...\n");

            if (queue_dequeue(q, id, sizeof(id)) == 0) {
                char name[MAX_NAME_LEN + 1];
                if (plist_get_name_by_id(pl, id, name, sizeof(name)) == 0) {
                    plist_set_called(pl, id, true);
                    printf("Chamando: ID %s | Nome: %s\n", id, name);
                } else
                    printf("Chamando: ID %s | Nome: (desconhecido)\n", id);
            } else
                printf("Fila vazia.\n");

            message_and_clear("Operação concluída. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 6) {
            /* Imprimir fila com nomes associados aos IDs */
            int qsize = queue_size(q);
            if (qsize == 0) {
                message_and_clear("Fila vazia. Retornando ao menu...", MSG_WAIT_SHORT);
            } else {
                printf("Fila de espera (total=%d):\n", qsize);
                for (int i = 0; i < qsize; ++i) {
                    char id[MAX_ID_LEN + 1];
                    char name[MAX_NAME_LEN + 1];
                    if (queue_get_id_by_index(q, i, id, sizeof(id)) != 0) continue;
                    if (plist_get_name_by_id(pl, id, name, sizeof(name)) != 0)
                        strncpy(name, "(desconhecido)", sizeof(name));
                    printf("%d: %s — %s\n", i + 1, id, name);
                }
                message_and_clear("Retornando ao menu...", MSG_WAIT_SHORT);
            }
        } else if (opc == 7) {
            char id[MAX_ID_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));

            if (plist_find_index(pl, id) < 0) {
                printf("Paciente não encontrado.\n");
                message_and_clear("Paciente não encontrado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            int n = plist_history_size_by_id(pl, id);
            char name[MAX_NAME_LEN + 1];
            plist_get_name_by_id(pl, id, name, sizeof(name));

            printf("Histórico de %s (ID %s): %d item(ns)\n", name, id, n);

            for (int i = 0; i < n; ++i) {
                char item[PROC_MAX_LEN + 1];
                if (plist_history_get_by_id(pl, id, i, item, sizeof(item)) == 0)
                    printf("%d) %s\n", i + 1, item);
            }

            message_and_clear("Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 8) {
            if (io_save(DATA_FILE, pl, q) == 0)
                printf("Dados salvos em %s. Até breve!\n", DATA_FILE);
            else
                printf("Erro ao salvar dados.\n");

            break;
        } else {
            printf("Opção inválida.\n");
            message_and_clear("Opção inválida. Retornando ao menu...", MSG_WAIT_SHORT);
        }
    }

    queue_destroy(q);
    plist_destroy(pl);

    return 0;
}