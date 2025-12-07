/* 
Este código contém a função main() da aplicação "postinho de saúde".
Objetivo geral:
- Gerir uma lista de pacientes (pl) e uma fila de espera (q).
- Fornecer um menu simples em linha de comando para registrar pacientes,
  registrar obito de paciente,
  adicionar/desfazer procedimentos no histórico,
  chamar o próximo, mostrar a fila, exibir histórico e salvar os dados ao sair.

Inclusões e módulos:
- config.h: constantes de configuração (tamanhos máximos, capacidade da fila).
- patient_list.h: interface para manipular a lista de pacientes (inserir, buscar, obter, liberar).
- queue.h: interface para fila de espera (inicializar, enfileirar, desenfileirar, verificar existência/cheia, liberar).
- history.h: interface para o histórico de procedimentos por paciente (push, pop, verificar cheio).
- io.h: funções para salvar/carregar dados persistentes (io_save, io_load).
- util.h: utilitários de I/O (read_line, util_printf, format_timestamp).
- clear_screen.h: header para limpar a tela

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

     2) Registrar óbito de paciente:
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
#include "patient_tree.h" // header da árvore de pacientes -> substituto de patient_list.h (razão: otimização máxima para sistemas mais sobrecarregados)
// #include "patient_list.h" // header da lista de pacientes
#include "priority_queue.h" // substitui queue.h
// #include "queue.h" // header da fila de espera
#include "history.h" // header do histórico
#include "io.h" // header para salvar/carregar
#include "util.h" // header com read_line()
#include "clear_screen.h" // header para limpar a tela

/* locale / widechar suporte para nomes acentuados */
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <stdbool.h>
#if defined(_WIN32)
#include <windows.h>
#endif

/* callback usado por show_archive_data para imprimir paciente + historico */
static void show_patient_and_history_cb(const char *id, const char *name, bool called, void *ud) {
    (void)called;
    PatientTree *pt = (PatientTree*)ud;
    printf(" - %s: %s\n", id, name);

    int hsz = ptree_history_size_by_id(pt, id);
    if (hsz > 0) {
        util_printf("\nHistorico de %s (ID %s): %d item(ns)\n", name, id, hsz);

        for (int hi = 0; hi < hsz; ++hi) {
            char hline[PROC_MAX_LEN + 1];

            if (ptree_history_get_by_id(pt, id, hi, hline, sizeof(hline)) == 0)
                util_printf("  %d) %s\n", hi + 1, hline);
        }

        util_printf("\n");
    }
}

/* callback C puro para listar pacientes */
static void list_cb(const char *id, const char *name, bool called, void *ud) {
    PatientTree *pt = (PatientTree*)ud;
    int pri = ptree_get_priority(pt, id);
    util_printf("- ID: %s | Nome: %s | Chamado: %s | Prioridade: P%d\n", id, name, called ? "SIM" : "NAO", pri);
}

/* Helpers locais para melhorar legibilidade */

/* Imprime o menu principal no terminal (sem lógica de leitura)
   Esta função só apresenta as opções disponíveis para o usuário */
static void show_menu(void) {
    printf("\nMenu:\n");
    printf("1. Registrar paciente\n");
    printf("2. Remover paciente\n");
    printf("3. Listar pacientes\n");
    printf("4. Buscar paciente por ID\n");
    printf("5. Chamar proximo paciente (por prioridade)\n");
    printf("6. Mostrar fila de espera\n");
    printf("7. Dar alta ao paciente\n");
    printf("8. Sair\n");
    printf("Escolha: ");
}

/* Mostra informações carregadas do ficheiro de dados:
   - lista de pacientes (resumo)
   - históricos por paciente
   - fila de espera com nomes resolvidos via PatientList
   Em caso de erro no carregamento, informa o usuário e inicializa vazio */
static void show_archive_data(PatientTree *pt, PriorityQueue *q, int load_r) {
    /* Tenta carregar dados persistidos (se existir) */
    if (load_r == 0) {
        /* Mostra resumidamente o estado carregado para o utilizador antes de limpar */
        printf("Dados carregados a partir de %s.\n\n", DATA_FILE);

        /* imprime cada paciente (em ordem) e seus historicos via callback */
        ptree_inorder(pt, show_patient_and_history_cb, pt);

        if (pqueue_size(q) > 0) {
            printf("Fila de espera:\n");
            int qsz = pqueue_size(q);

            for (int qi = 0; qi < qsz; ++qi) {
                char qid[MAX_ID_LEN + 1];
                char qname[MAX_NAME_LEN + 1];
                int prio = 0;

                if (pqueue_get_id_by_index(q, qi, qid, sizeof(qid)) != 0) 
                    continue;

                if (ptree_get_name(pt, qid, qname, sizeof(qname)) != 0)
                    strncpy(qname, "(desconhecido)", sizeof(qname));

                (void)pqueue_get_priority_by_index(q, qi, &prio);
                util_printf("%d: %s - %s (P%d)\n", qi + 1, qid, qname, prio);
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
    }
    
    else if (load_r == -1) {
        /* arquivo inexistente: inicialização com estruturas vazias (normal em primeira execução) */
        printf("Nenhum arquivo de dados encontrado.\n");
        message_and_clear("Iniciando com banco vazio.", MSG_WAIT_SHORT);

    } 
    
    else {
        printf("Erro ao carregar dados (formato/IO).\n");
        message_and_clear("Iniciando com banco vazio.", MSG_WAIT_SHORT);
    }

    /* Mensagem de boas-vindas simples (sem limpar novamente de forma imediata) */
    message_and_clear("Bem-vindo ao PostinhoSUS - Sistema de Gestao (Projeto AED, ICMC 2025).", MSG_WAIT_SHORT);
}

/* Funcao principal do programa simplificada:
   - inicializa estruturas (PatientList, Queue)
   - tenta carregar dados persistidos (io_load)
   - executa loop de menu interpretando as opcoes 1..7
   - delega operacoes aos modulos (ptree_*, queue_*, history_*, io_*)
   - salva condicionalmente e libera recursos antes de terminar
*/

int main(){
    /* configuração local para suporte à dados multibyte (acentuados) */
    util_setup_locale();

    /* Usamos a árvore diretamente em runtime */
    PatientTree *pt = ptree_create();
    PriorityQueue *q  = pqueue_create(WAIT_CAP);
    
    if (!pt || !q) {
        fprintf(stderr, "Erro de inicializacao.\n");
        return 1;
    }
    
    /* Carrega diretamente na árvore (antes carregávamos numa PatientList e convertíamos) */
    int load_r = io_load(DATA_FILE, pt, q);
    /* show_archive_data agora aceita a árvore (comparativo com antigo uso de PatientList permanece em comentários) */
    show_archive_data(pt, q, load_r);

    int opc = 0;
    char buf[256];

    for (;;) {
        show_menu();

        if (!fgets(buf, sizeof(buf), stdin))
            break;

        opc = atoi(buf);

        clear_screen(); // limpa a tela após a escolha

        if (opc == 1) { // Registrar paciente
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

                size_t end = util_strnlen(id, MAX_ID_LEN + 1);
                while (end > start && isspace((unsigned char)id[end - 1]))
                    --end;

                if (start != 0 || end != strlen(id)) {
                    /* compacta a string */
                    size_t len = end - start;
                    if (len > 0) memmove(id, id + start, len);
                    id[len] = '\0';
                }

                size_t len = util_strnlen(id, MAX_ID_LEN + 1);
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

                /* Se o ID já está cadastrado (antigo: plist_find_index/plist_*), agora usamos ptree_exists, mas ainda oferecer reinsercao na fila:
                   - se já estiver na fila, avisa e retorna ao menu
                   - se a fila já estiver cheia, avisa e retorna ao menu
                   - senao, enfileira e informa "paciente reinserido na fila!" */
                // if (plist_find_index(pl, id) >= 0) {
                if (ptree_exists(pt, id)) {
                    if (pqueue_contains(q, id)) {
                        message_and_clear("Paciente ja esta na fila. Retornando ao menu...", MSG_WAIT_SHORT);
                        reinInserted = 1; /* tratou a situacao, não cadastrar */
                        break;
                    } 
                    
                    else if (pqueue_is_full(q)) {
                        message_and_clear("Fila cheia. Nao foi possivel inserir.", MSG_WAIT_SHORT);
                        reinInserted = 1; /* não cadastrar */
                        break;
                    } 
                    
                    else {
                        int pri;
                        for (;;) {
                            printf("Prioridade (1 = Emergencia, 2 = Muito urgente, 3 = Urgente, 4 = Pouco urgente, 5 = Nao urgencia): ");
                            char pbuf[16];
                            read_line(pbuf, sizeof(pbuf));
                            pri = atoi(pbuf);
                            if (pri >= 1 && pri <= 5)
                                break;

                            message_and_clear("Prioridade invalida.", MSG_WAIT_SHORT);
                        }

                        pqueue_enqueue(q, id, pri);
                        (void)ptree_set_called(pt, id, false);
                        (void)ptree_set_priority(pt, id, pri);
                        (void)ptree_set_discharged(pt, id, false); /* reset alta ao reinserir na fila */
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

            /* ORIGINAL (lista):
                int r = plist_insert(pl, id, name);
            */
            int r = ptree_insert(pt, id, name); /* agora usa árvore */

            if (r == 0)
                printf("Paciente cadastrado.\n");

            else {
                printf("Falha ao cadastrar.\n");
                message_and_clear("Falha ao cadastrar. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (pqueue_is_full(q))
                printf("Fila cheia. Nao foi possivel inserir.\n");

            else if (pqueue_contains(q, id))
                printf("Paciente ja esta na fila de espera.\n");
                
            else {
                int pri;

                for(;;){
                    printf("Prioridade (1 = Emergencia, 2 = Muito urgente, 3 = Urgente, 4 = Pouco urgente, 5 = Nao urgencia): ");
                    char pbuf[16];
                    read_line(pbuf, sizeof(pbuf));
                    pri = atoi(pbuf);
                    if (pri >= 1 && pri <= 5)
                        break;

                    message_and_clear("Prioridade invalida.", MSG_WAIT_SHORT);
                }

                pqueue_enqueue(q, id, pri);
                (void)ptree_set_priority(pt, id, pri);
                printf("Paciente inserido na fila.\n");
            }

            message_and_clear("Operacao concluida. Retornando ao menu...", MSG_WAIT_SHORT);

        } 
        
        else if (opc == 2) { // Remover paciente. Para o trabalho, foi escolhido um cenário ideal em que o paciente só morreria, caso não estivesse na fila.
            char id[MAX_ID_LEN + 1];
            printf("ID do paciente a remover (obito): "); read_line(id, sizeof(id));

            /* trim de espaços (evita falha por espaços acidentais) */
            {
                size_t start = 0;

                while (id[start] && isspace((unsigned char)id[start])) 
                    ++start;

                size_t end = util_strnlen(id, sizeof(id));

                while (end > start && isspace((unsigned char)id[end - 1])) 
                    --end;

                if (start != 0 || end != strlen(id)) {
                    size_t len = end - start;

                    if (len > 0) 
                        memmove(id, id + start, len);

                    id[len] = '\0';
                }
            }

            /* DEBUG: imprimir id lido e garantir flush */
            util_printf("DEBUG: tentar remover id='%s'\n", id);
            fflush(stdout);

            if (!ptree_exists(pt, id)) {
                message_and_clear("Paciente nao encontrado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            /* checar se está na fila */
            if (pqueue_contains(q, id)) {
                printf("Obito proibido.\n");
                message_and_clear("Paciente ainda esta na fila. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            /* verificar flag 'called' */
            // Original (lista): if (!plist_is_called(pl, id)) { ... }
            {
                int called = ptree_is_called(pt, id);
                fflush(stdout);

                if (!called) {
                    message_and_clear("Paciente nao foi chamado. Retornando ao menu...", MSG_WAIT_SHORT);
                    continue;
                }
            }

            /* tenta remover e imprime codigo de retorno para diagnostico */
            int rem_rc = ptree_remove(pt, id);
            util_printf("DEBUG: ptree_remove('%s') retornou %d\n", id, rem_rc);
            fflush(stdout);

            if (rem_rc == 0)
                message_and_clear("Obito registrado e dados removidos (LGPD). Retornando ao menu...", MSG_WAIT_MEDIUM);

            else {
                util_printf("Falha ao remover registro do paciente. codigo=%d\n", rem_rc);
                message_and_clear("Falha ao remover registro do paciente. Retornando ao menu...", MSG_WAIT_MEDIUM);
            }

            message_and_clear("Operacao concluida. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } 
        
        else if (opc == 3) { // Listar pacientes
            /* Listar pacientes: percorre a árvore e imprime resumo (id | nome | chamados?) */
            util_printf("Lista de pacientes registrados (ordenada por ID):\n");

            /* Uso de callback C puro */
            ptree_inorder(pt, list_cb, pt);
            message_and_clear("Retornando ao menu...", MSG_WAIT_SHORT);

        } 
        
        else if (opc == 4) { // Buscar paciente por ID
            /* Abre-se um submenu com operacoes relacionadas:
               - 1: Adicionar procedimento
               - 2: Desfazer ultimo procedimento
               - 3: Mostrar historico
               - 4: Voltar ao menu principal
            */
            
            char id[MAX_ID_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));

            /* ORIGINAL (lista): if (plist_find_index(pl, id) < 0) { ... } */
            if (!ptree_exists(pt, id)) {
                message_and_clear("Paciente nao encontrado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            /* imprime dados basicos e histórico resumido */
            char name[MAX_NAME_LEN + 1];
            if (ptree_get_name(pt, id, name, sizeof(name)) != 0)
                strncpy(name, "(desconhecido)", sizeof(name));

            int called = ptree_is_called(pt, id);
            util_printf("Nome: %s | Chamado: %s\n", name, called ? "SIM" : "NAO");

            /* menu interno de acoes sobre o paciente */
            for (;;) {
                printf("\nOperacoes para ID %s:\n", id);
                printf("1. Adicionar procedimento\n");
                printf("2. Desfazer ultimo procedimento\n");
                printf("3. Mostrar historico completo\n");
                printf("4. Voltar\n");
                printf("Escolha: ");

                char subbuf[64];

                if (!fgets(subbuf, sizeof(subbuf), stdin)) 
                    break;

                int sub = atoi(subbuf);
                clear_screen();

                if (sub == 1) {
                    /* Adicionar procedimento (reaproveita validacoes existentes) */
                    if (ptree_history_is_full(pt, id)) {
                        message_and_clear("Historico cheio. Retornando ao submenu...", MSG_WAIT_SHORT);
                        continue;
                    }

                    /* lê descrição com validações e prefixa timestamp via util::format_timestamp*/
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

                    /* obtêm timestamp formatado (se disponível)*/
                    char timestr[32] = {0};

                    if (format_timestamp(timestr, sizeof(timestr)) != 0)
                        timestr[0] = '\0';

                    /* monta item com timestamp seguro e truncado para PROC_MAX_LEN */
                    char item[PROC_MAX_LEN + 1];
                    item[0] = '\0';

                    if (timestr[0] != '\0') {
                        /* escreve somente o prefixo "[timestr]" e depois concatena o proc
                           limitando a cópia ao espaço restante, para evitar warnings do compilador */
                        int pref = snprintf(item, sizeof(item), "[%s] ", timestr);

                        if (pref < 0) 
                            pref = 0;

                        size_t used = (size_t)pref;

                        if (used >= sizeof(item))
                            /* já cheio, garante terminação */
                            item[sizeof(item) - 1] = '\0';
                        
                        else {
                            size_t avail = sizeof(item) - used - 1; // espaço restante para chars + '\0
                            strncat(item, proc, avail);
                            // strncat usa o espaço disponível, garantindo terminação
                        }

                    } 
                    
                    else {
                        // sem timestamp: copia procedure com segurança
                        strncpy(item, proc, sizeof(item) - 1);
                        item[sizeof(item) - 1] = '\0';
                    }

                    // Original (lista): plist_history_push(pl, id, item)
                    if (ptree_history_push(pt, id, item) == 0)
                        printf("Procedimento adicionado.\n");

                    else
                        printf("Falha ao adicionar procedimento (historico cheio ou erro).\n");

                    message_and_clear("Operacao concluida. Retornando ao submenu...", MSG_WAIT_MEDIUM);

                } 
                
                else if (sub == 2) {// Desfazer ultimo procedimento
                    char out[PROC_MAX_LEN + 1];

                    if (ptree_history_pop(pt, id, out, sizeof(out)) == 0)
                        printf("Procedimento desfeito (ultimo): %s\n", out);

                    else
                        printf("Nao ha procedimento a desfazer\n");

                    message_and_clear("Retornando ao submenu...", MSG_WAIT_MEDIUM);

                } 
                
                else if (sub == 3) { /* Mostrar historico completo */
                    int n = ptree_history_size_by_id(pt, id);
                    util_printf("Historico de %s (ID %s): %d item(ns)\n", name, id, n);

                    for (int i = 0; i < n; ++i) {
                        char item[PROC_MAX_LEN + 1];

                        if (ptree_history_get_by_id(pt, id, i, item, sizeof(item)) == 0)
                            util_printf("%d) %s\n", i + 1, item);
                    }
                    
                    message_and_clear("Retornando ao submenu...", MSG_WAIT_MEDIUM);

                } 
                
                else if (sub == 4) // voltar ao menu principal
                    break;

                else {
                    printf("Opcao invalida.\n");
                    message_and_clear("Opcao invalida. Retornando ao submenu...", MSG_WAIT_SHORT);
                }
            }

            /* ao sair do submenu, retorna ao loop principal */
            continue;

        } 
        
        else if (opc == 5) { // chamar próximo por prioridade
            char id[MAX_ID_LEN + 1];

            if (pqueue_size(q) == 0)
                message_and_clear("Fila vazia. Ninguem para chamar.", MSG_WAIT_SHORT);
            
            else if (pqueue_dequeue(q, id, sizeof(id)) == 0) {
                char name[MAX_NAME_LEN + 1];

                if (ptree_get_name(pt, id, name, sizeof(name)) != 0)
                    strncpy(name, "(desconhecido)", sizeof(name));

                (void)ptree_set_called(pt, id, true);
                util_printf("Chamando: %s - %s\n", id, name);
                message_and_clear("Paciente chamado. Retornando ao menu...", MSG_WAIT_SHORT);
            } 
            
            else
                message_and_clear("Falha ao chamar proximo paciente.", MSG_WAIT_SHORT);
        } 
        
        else if (opc == 6) { // Mostrar fila
            /* Mostrar fila de espera (reaproveita código existente) */
            int qsize = pqueue_size(q);

            if (qsize == 0)
                message_and_clear("Fila vazia. Retornando ao menu...", MSG_WAIT_SHORT);
            
            else {
                util_printf("Fila de espera (total = %d):\n", qsize);

                for (int i = 0; i < qsize; ++i) {
                    char id[MAX_ID_LEN + 1];
                    char name[MAX_NAME_LEN + 1];
                    int pri = 0;

                    if (pqueue_get_id_by_index(q, i, id, sizeof(id)) != 0) 
                        continue;

                    if (ptree_get_name(pt, id, name, sizeof(name)) != 0)
                        strncpy(name, "(desconhecido)", sizeof(name));

                    (void)pqueue_get_priority_by_index(q, i, &pri);
                    util_printf("%d: %s - %s (P%d)\n", i + 1, id, name, pri);
                }

                message_and_clear("Retornando ao menu...", MSG_WAIT_SHORT);
            }

        } else if (opc == 7) {
            /* Dar alta ao paciente: remove registro se já foi chamado e não estiver na fila */
            char id[MAX_ID_LEN + 1];
            /* ORIGINAL (lista): printf("ID para dar alta: "); read_line(id, sizeof(id)); */
            printf("ID para dar alta: "); read_line(id, sizeof(id));

            /* trim de espaços (mesma razão que em opcao 2) */
            {
                size_t start = 0;

                while (id[start] && isspace((unsigned char)id[start])) 
                    ++start;

                size_t end = util_strnlen(id, sizeof(id));

                while (end > start && isspace((unsigned char)id[end - 1])) 
                    --end;

                if (start != 0 || end != strlen(id)) {
                    size_t len = end - start;

                    if (len > 0) 
                        memmove(id, id + start, len);

                    id[len] = '\0';
                }
            }

            if (!ptree_exists(pt, id)) {
                printf("Paciente nao encontrado.\n");
                message_and_clear("Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (pqueue_contains(q, id)) {
                message_and_clear("Paciente ainda esta na fila. Nao e possivel dar alta.", MSG_WAIT_SHORT);
                continue;
            }

            if (!ptree_is_called(pt, id)) {
                message_and_clear("Paciente nao foi chamado. Operacao de alta proibida.", MSG_WAIT_SHORT);
                continue;
            }

            /* Registrar alta no historico e resetar flag 'called' */
            {
                char item[PROC_MAX_LEN + 1];
                char timestr[32] = {0};
                (void)format_timestamp(timestr, sizeof(timestr));
                if (timestr[0] != '\0')
                    snprintf(item, sizeof(item), "[%s] Alta concedida", timestr);
                else
                    strncpy(item, "Alta concedida", sizeof(item) - 1), item[sizeof(item) - 1] = '\0';

                /* tentar registrar a alta no histórico; ignorar erro se cheio */
                (void)ptree_history_push(pt, id, item);
            }

            (void)ptree_set_called(pt, id, false);
            (void)ptree_set_discharged(pt, id, true); /* marcar alta persistente */
            message_and_clear("Alta concedida. Registro mantido. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } else if (opc == 8) {
            /* Sair: salvar e terminar 
               Agora gravamos diretamente a partir da árvore.
               Comentário comparativo (antigo): salvar via PatientList era:
                 // size_t n_pat = plist_size(pl);
                 // if (io_save(DATA_FILE, pl, q) == 0) ...
            */
            size_t n_pat = ptree_size(pt);
            int qsize = pqueue_size(q);

            if (n_pat > 0 || qsize > 0 || load_r == 0) {
                if (io_save(DATA_FILE, pt, q) == 0)
                    printf("Dados salvos em %s. Ate breve.\n", DATA_FILE);

                else
                    printf("Erro ao salvar dados.\n");
            } 
            
            else
                printf("Nenhum dado para salvar. Arquivo nao foi alterado.\n");

            break;

        } 
        
        else {
            printf("Opcao invalida.\n");
            message_and_clear("Retornando ao menu...", MSG_WAIT_SHORT);
        }
    }

    /* cleanup final: usar árvore + fila */
    ptree_destroy(pt);
    pqueue_destroy(q);

    return 0;
}