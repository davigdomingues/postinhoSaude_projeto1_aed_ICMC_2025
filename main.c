/* 
Este código contém a função main() da aplicação "postinho de saúde".
Objetivo geral:
- Gerir uma árvore de pacientes (PatientTree, pt) e uma fila de prioridades (PriorityQueue, q).
- Fornecer um menu simples em linha de comando para:
  cadastrar pacientes, registrar óbito, adicionar/desfazer procedimentos no histórico,
  chamar o próximo por prioridade, mostrar a fila, exibir histórico e salvar os dados ao sair.

Módulos utilizados:
- config.h: constantes de configuração (tamanhos máximos, capacidade da fila, caminhos).
- patient_tree.h: TAD de pacientes em árvore AVL (inserir, remover, buscar, flags, prioridade, histórico).
- priority_queue.h: TAD fila de prioridades (enfileirar/desenfileirar, acesso por índice, consulta de existência).
- history.h: histórico de procedimentos por paciente (push/pop/size/get_by_index).
- io.h: persistência textual: io_save/io_load para PatientTree + PriorityQueue (compatível com UTF-8).
- util.h: utilitários (read_line, util_printf UTF-8, format_timestamp, locale).
- clear_screen.h: utilidades de UI (limpar tela e mensagens temporizadas).

Estruturas usadas em runtime (alocadas dinamicamente):
- PatientTree *pt; // árvore AVL de pacientes
- PriorityQueue *q; // fila de prioridades (1..5)

Fluxo principal (main):
1. Inicialização:
   - util_setup_locale(): prepara ambiente/console para UTF-8.
   - ptree_create(): cria árvore de pacientes.
   - pqueue_create(WAIT_CAP): cria fila de prioridades com capacidade definida em config.h.
   - io_load(DATA_FILE, pt, q): carrega dados (pacientes, históricos e fila).

2. Loop do menu:
   - Exibe opções numeradas de 1 a 8.
   - Lê uma linha com fgets() para evitar estouro de buffer e converte para inteiro com atoi().
   - Cada opção chama funções dos módulos correspondentes e realiza verificações:

     1) Registrar paciente:
        - Lê ID (somente dígitos).
        - Se já existe: permitir reinserção na fila, apenas se foi chamado e recebeu alta; atualiza prioridade e flags.
        - Senão: insere na árvore (ptree_insert) e pode enfileirar com prioridade informada (pqueue_enqueue).
        - Para prioridades 1..3, coleta "Razão da urgência" e registra no histórico.

     2) Registrar óbito:
        - Lê ID, verifica se não está na fila (pqueue_contains) e se foi chamado (ptree_is_called).
        - Remove o paciente da árvore (ptree_remove).

     3) Listar pacientes:
        - Percorre a árvore em ordem (ptree_inorder) e imprime resumo (ID, nome, chamado, prioridade).

     4) Buscar paciente por ID (submenu):
        - Mostra dados básicos e estado de hospital/fila.
        - Submenu disponível apenas se foi chamado; permite adicionar/desfazer e ver histórico completo.

     5) Chamar próximo por prioridade:
       - pqueue_dequeue(&pq, id, sizeof(id)) remove o próximo da fila e coloca o ID em 'id'.
        - Marca chamado (ptree_set_called(pt, id, true)) e informa nome.
        - Usa ptree_get para tentar recuperar o nome do paciente (pode ser NULL se o cadastro não existir).

     6) Mostrar fila:
        - lista a fila resolvendo nomes via PatientTree (sem usar função de impressão do TAD).
        - Se a fila estiver vazia, avisa o usuário.

     7) Dar alta:
        - Somente se chamado, não está na fila e possui histórico >= 1.
        - Registra alta no histórico com timestamp, reseta chamado = false e marca discharged = true.

     8) Sair:
        - io_save(DATA_FILE, pt, q) persiste pacientes (com flags e prioridades), históricos e fila.

Convenções e I/O:
- util_printf deve ser usado para strings com acentuação (UTF-8).
- read_line/read_line_truncated protegem contra overflow e truncamento.
- Módulos retornam 0 em sucesso e !=0 em erro (convenção adotada no main).
- Muitas funções retornam 0 em caso de sucesso e valor != 0 em erro - o main assume essa convenção.
- fgets() é usado para ler entrada do utilizador e evitar overflow; atoi() para converter a opção.
- Variáveis temporárias (id, name) usam tamanhos definidos em config.h (MAX_ID_LEN, MAX_NAME_LEN, PROC_MAX_LEN).
- Mensagens informativas são exibidas via util_printf() ao usuário, em cada caminho de execução.

Limpeza:
- pqueue_destroy(q) e ptree_destroy(pt) liberam recursos dinâmicos.

Persistência:
- io_load carrega PatientTree e PriorityQueue.
- io_save grava pacientes (id, nome, histórico, chamado, prioridade, discharged), e a fila (id + prioridade).

Observações de integração:
- A maior parte da lógica "pesada" (pesquisa, memória, histórico) está em módulos separados (patient_tree, priority_queue, history, io, util, entre outros).

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

/* Helpers locais para melhorar legibilidade */

/* callback usado por show_archive_data para imprimir paciente + historico */
static void show_patient_and_history_cb(const char *id, const char *name, bool called, void *ud) {
    (void)called;
    PatientTree *pt = (PatientTree*)ud;
    util_printf(" - %s: %s\n", id, name);

    int hsz = ptree_history_size_by_id(pt, id);
    
    if (hsz > 0) {
        util_printf("\nHistórico de %s (ID %s): %d item(ns)\n", name, id, hsz);

        for (int hi = 0; hi < hsz; ++hi) {
            char hline[PROC_MAX_LEN + 1];

            if (ptree_history_get_by_id(pt, id, hi, hline, sizeof(hline)) == 0)
                util_printf("  %d) %s\n", hi + 1, hline);
        }

        util_printf("\n");
    }
    
    else {
        /* indica explicitamente que o histórico está vazio */
        util_printf("   (histórico vazio)\n\n");
    }
}

/* callback C puro para listar pacientes
static void list_cb(const char *id, const char *name, bool called, void *ud) {
    PatientTree *pt = (PatientTree*)ud;
    int pri = ptree_get_priority(pt, id);
    util_printf("- ID: %s | Nome: %s | Chamado: %s | Prioridade: P%d\n", id, name, called ? "SIM" : "NÃO", pri);
} */

/* Imprime o menu principal no terminal (sem lógica de leitura)
   Esta função só apresenta as opções disponíveis para o usuário */
static void show_menu(void) {
    util_printf("\nMenu:\n");
    util_printf("1. Registrar paciente\n");
    util_printf("2. Remover paciente\n");
    util_printf("3. Listar pacientes\n");
    util_printf("4. Buscar paciente por ID\n");
    util_printf("5. Chamar próximo paciente (por prioridade)\n");
    util_printf("6. Mostrar fila de espera\n");
    util_printf("7. Dar alta ao paciente\n");
    util_printf("8. Sair\n");
    util_printf("Escolha: ");
}

/* Mostra informações carregadas do ficheiro de dados:
   - árvore de pacientes (resumo)
   - históricos por paciente
   Em caso de erro no carregamento, informa o usuário e inicializa vazio */
static void show_archive_data(PatientTree *pt, int load_r) {
    /* Tenta carregar dados persistidos (se existir) */
    if (load_r == 0) {
        /* Mostra resumidamente o estado carregado para o utilizador antes de limpar, mostra resumidamente o estado carregado para o utilizador antes de limpar */
        util_printf("Dados carregados a partir de %s.\n\n", DATA_FILE);
        /* Lista somente pacientes e histórico */
        ptree_inorder(pt, show_patient_and_history_cb, pt);

        /* Em vez de limpar automaticamente, aguarda que o utilizador pressione Enter
           para garantir que as impressões permaneçam visíveis. */

        {
            char __tmp_wait[8];
            util_printf("\nPressione Enter para continuar...");
            fflush(stdout);
            read_line(__tmp_wait, sizeof(__tmp_wait));
            clear_screen();
        }
    } 
    
    else if (load_r == -1) {
        /* arquivo inexistente: inicialização com estruturas vazias (normal em primeira execução) */
        util_printf("Nenhum arquivo de dados encontrado.\n");
        message_and_clear("Iniciando com banco vazio.", MSG_WAIT_SHORT);

    } 
    
    else {
        /* erro ao carregar dados (formato/IO) */
        message_and_clear("Erro ao carregar os dados. Considerar o banco vazio.", MSG_WAIT_SHORT);
    }
}

/* Funcao principal do programa simplificada:
   - inicializa estruturas (PatientTree, PriorityQueue)
   - tenta carregar dados persistidos (io_load)
   - executa loop de menu interpretando as opcoes 1..7
   - delega operacoes aos modulos (ptree_*, pqueue_*, history_*, io_*)
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

    /* Mensagem de boas-vindas antes de iniciar o loop do menu */
    message_and_clear("Bem-vindo ao PostinhoSUS - Sistema de Gestão (Projeto AED, ICMC 2025).", MSG_WAIT_SHORT);

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
                util_printf("ID (somente dígitos): ");
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

                /* Se o ID já está cadastrado (antigo: plist_find_index/plist_*), agora usamos ptree_exists, mas ainda oferecer reinserção na fila:
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

                    /* só permite reinserção se foi chamado e recebeu alta */
                    else {
                        int was_called = ptree_is_called(pt, id);
                        int was_discharged = ptree_is_discharged(pt, id);
                        
                        if (!was_called || !was_discharged) {
                            message_and_clear("Reinsercao negada: paciente precisa ter sido chamado e ter recebido alta.", MSG_WAIT_SHORT);
                            reinInserted = 1;
                            break;
                        }

                        int pri;
                        
                        for (;;) {
                            util_printf("Prioridade (1 = Emergência, 2 = Muito urgente, 3 = Urgente, 4 = Pouco urgente, 5 = Não urgência): ");
                            char pbuf[16];
                            read_line(pbuf, sizeof(pbuf));
                            pri = atoi(pbuf);
                            
                            if (pri >= 1 && pri <= 5)
                                break;

                            message_and_clear("Prioridade invalida.", MSG_WAIT_SHORT);
                        }

                        /* coleta da razão de urgência para prioridades 1..3 e registrar no histórico */
                        char item[PROC_MAX_LEN + 1];
                        if (pri >= 1 && pri <= 3) {
                            for (;;) {
                                if (ptree_history_is_full(pt, id)) {
                                    message_and_clear("Historico cheio. Retornando ao submenu...", MSG_WAIT_SHORT);
                                    continue;
                                }

                                /* lê descrição com validações e prefixa timestamp via util::format_timestamp*/
                                char proc[PROC_MAX_LEN + 1];
                                util_printf("Razão da urgência (ate %d chars): ", PROC_MAX_LEN); /* util_printf para acentos */
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

                                break;
                            }
                        }

                        if (pqueue_enqueue(q, id, pri) == 0) {
                            (void)ptree_set_called(pt, id, false);
                            (void)ptree_set_priority(pt, id, pri);
                            (void)ptree_set_discharged(pt, id, false); /* reset alta ao reinserir */

                            if (pri >= 1 && pri <= 3) {
                                if (ptree_history_push(pt, id, item) == 0)
                                    util_printf("Procedimento adicionado.\n");
                                else
                                    util_printf("Falha ao adicionar procedimento (historico cheio ou erro).\n");
                            }

                            message_and_clear("Paciente reinserido na fila apos alta.", MSG_WAIT_SHORT);

                            /* persistência imediata após reinserção */
                            (void)io_save(DATA_FILE, pt, q);
                            reinInserted = 1; // ja reinserido, pular cadasatro
                            break;
                        } 
                        
                        else {
                            message_and_clear("Falha ao reinserir na fila.", MSG_WAIT_SHORT);
                            reinInserted = 1;
                            break;
                        }
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
                util_printf("Paciente cadastrado.\n");

            else {
                util_printf("Falha ao cadastrar.\n");
                message_and_clear("Falha ao cadastrar. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            if (pqueue_is_full(q))
                util_printf("Fila cheia. Não foi possível inserir.\n");
            else if (pqueue_contains(q, id))
                util_printf("Paciente já está na fila de espera.\n");
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

                char item[PROC_MAX_LEN + 1];

                if(pri>=1 && pri<=3) {
                    for(;;) {
                            /* lê descrição com validações e prefixa timestamp via util::format_timestamp*/
                            char proc[PROC_MAX_LEN + 1];
                            util_printf("Razão da urgência (ate %d chars): ", PROC_MAX_LEN); /* util_printf para acentos */
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

                            break;
                    }
                }

                pqueue_enqueue(q, id, pri);
                (void)ptree_set_priority(pt, id, pri);
                if(pri>=1 && pri<=3) {
                    if (ptree_history_push(pt, id, item) == 0)
                        util_printf("Procedimento adicionado.\n");

                    else
                        util_printf("Falha ao adicionar procedimento (histórico cheio ou erro).\n");
                }

                util_printf("Paciente inserido na fila.\n");

                /* persistência imediata após cadastro/entrada na fila */
                (void)io_save(DATA_FILE, pt, q);
            }

            message_and_clear("Operacao concluida. Retornando ao menu...", MSG_WAIT_SHORT);

        } 
        
        else if (opc == 2) { // Remover paciente (óbito). Para o trabalho, foi escolhido um cenário ideal em que o paciente só morreria, caso não estivesse na fila.
            char id[MAX_ID_LEN + 1];
            util_printf("ID do paciente a remover (óbito): ");
            read_line(id, sizeof(id));

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

            /* DEBUG: imprimir id lido e garantir flush 
            util_printf("DEBUG: tentar remover id='%s'\n", id); */
            fflush(stdout);

            if (!ptree_exists(pt, id)) {
                message_and_clear("Paciente nao encontrado. Retornando ao menu...", MSG_WAIT_SHORT);
                continue;
            }

            /* checar se está na fila */
            if (pqueue_contains(q, id)) {
                util_printf("Obito proibido.\n");
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
            // util_printf("DEBUG: ptree_remove('%s') retornou %d\n", id, rem_rc);
            fflush(stdout);

            if (rem_rc == 0)
                message_and_clear("Obito registrado e dados removidos (LGPD). Retornando ao menu...", MSG_WAIT_MEDIUM);

            else {
                util_printf("Falha ao remover registro do paciente. código = %d\n", rem_rc);
                message_and_clear("Falha ao remover registro do paciente. Retornando ao menu...", MSG_WAIT_MEDIUM);
            }

            /* persistência imediata após remoção/óbito (mesmo em falha não altera estado, salvar só em sucesso) */
            if (rem_rc == 0) (void)io_save(DATA_FILE, pt, q);
            message_and_clear("Operacao concluida. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } 
        
        else if (opc == 3) { // Listar pacientes (snapshot atual de DATA_FILE)
            /* Mostra o que está gravado AGORA em DATA_FILE:
               cria estruturas temporárias, carrega e exibe apenas pacientes + histórico. */
            PatientTree *pt_tmp = ptree_create();
            PriorityQueue *q_tmp = pqueue_create(WAIT_CAP); /* apenas para io_load; não exibimos a fila */

            if (!pt_tmp || !q_tmp) {
                if (pt_tmp) 
                    ptree_destroy(pt_tmp);

                if (q_tmp) 
                    pqueue_destroy(q_tmp);
                
                message_and_clear("Falha ao alocar estruturas temporarias.", MSG_WAIT_SHORT);

                /* fallback: lista estado atual em memória (somente pacientes e histórico) */
                util_printf("Pacientes registrados (ordenados por ID) e histórico:\n");
                ptree_inorder(pt, show_patient_and_history_cb, pt);
                {
                    char __tmp_wait[8];
                    util_printf("\nPressione Enter para retornar ao menu");
                    fflush(stdout);
                    read_line(__tmp_wait, sizeof(__tmp_wait));
                    clear_screen();
                }
                
                continue;
            }

            int lr_now = io_load(DATA_FILE, pt_tmp, q_tmp);
            show_archive_data(pt_tmp, lr_now);

            ptree_destroy(pt_tmp);
            pqueue_destroy(q_tmp);
            /* show_archive_data já aguarda Enter e limpa a tela; voltar ao menu */
        } 
        
        else if (opc == 4) { // Buscar paciente por ID (submenu)
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

            /* imprime dados basicos, histórico resumido e estado de internacao/fila */
            char name[MAX_NAME_LEN + 1];

            if (ptree_get_name(pt, id, name, sizeof(name)) != 0)
                strncpy(name, "(desconhecido)", sizeof(name));

            int called = ptree_is_called(pt, id);
            int in_queue = pqueue_contains(q, id);

            util_printf("Nome: %s | Chamado: %s | Estado: %s%s\n",
                        name,
                        called ? "SIM" : "NAO",
                        "No hospital",
                        in_queue ? " e NA FILA" : "");

            /* submenu somente se foi chamado */
            if (!called) {
                message_and_clear("Paciente ainda nao foi chamado. Submenu indisponivel.", MSG_WAIT_SHORT);
                continue;
            }

            /* menu interno de acoes sobre o paciente (disponivel apenas se chamado) */
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
                    /* Adicionar procedimento: permitido apenas porque chamado==true (gated acima), reaproveita validações existentes */
                    if (ptree_history_is_full(pt, id)) {
                        message_and_clear("Historico cheio. Retornando ao submenu...", MSG_WAIT_SHORT);
                        continue;
                    }

                    /* lê descrição com validações e prefixa timestamp via util::format_timestamp*/
                    char proc[PROC_MAX_LEN + 1];
                    printf("Procedimento (ate %d chars): ", PROC_MAX_LEN); /* prompt pode ficar em printf ASCII */
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
                        util_printf("Procedimento adicionado.\n");

                    else
                        util_printf("Falha ao adicionar procedimento (histórico cheio ou erro).\n");
                        
                    /* salvar alteração de histórico imediatamente */
                    (void)io_save(DATA_FILE, pt, q);
                    message_and_clear("Operacao concluida. Retornando ao submenu...", MSG_WAIT_MEDIUM);

                } 
                
                else if (sub == 2) {// Desfazer ultimo procedimento
                    char out[PROC_MAX_LEN + 1];

                    if (ptree_history_pop(pt, id, out, sizeof(out)) == 0)
                        util_printf("Procedimento desfeito (último): %s\n", out);
                        
                    else
                        util_printf("Não há procedimento a desfazer\n");
                    
                    /* salvar alteração de histórico imediatamente */
                    (void)io_save(DATA_FILE, pt, q);
                    message_and_clear("Retornando ao submenu...", MSG_WAIT_MEDIUM);

                } 
                
                else if (sub == 3) { /* Mostrar historico completo */
                    int n = ptree_history_size_by_id(pt, id);
                    util_printf("Histórico de %s (ID %s): %d item(ns)\n", name, id, n);

                    for (int i = 0; i < n; ++i) {
                        char item[PROC_MAX_LEN + 1];

                        if (ptree_history_get_by_id(pt, id, i, item, sizeof(item)) == 0)
                            util_printf("%d) %s\n", i + 1, item);
                    }
                    
                    {
                        char __tmp_wait[8];
                        util_printf("\nPressione Enter para retornar ao submenu");
                        fflush(stdout);
                        read_line(__tmp_wait, sizeof(__tmp_wait));
                        clear_screen();
                    }
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
                
                /* salvar estado de fila e flag 'called' */
                (void)io_save(DATA_FILE, pt, q);
                message_and_clear("Paciente chamado. Retornando ao menu...", MSG_WAIT_SHORT);
            } 
            
            else
                message_and_clear("Falha ao chamar próximo paciente.", MSG_WAIT_SHORT);
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

                {
                    char __tmp_wait[8];
                    util_printf("\nPressione Enter para retornar ao menu");
                    fflush(stdout);
                    read_line(__tmp_wait, sizeof(__tmp_wait));
                    clear_screen();
                }
            }

        } 
        
        else if (opc == 7) {
            /* Dar alta ao paciente: apenas se foi chamado, nao esta na fila e possui historico >= 1 */
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
                util_printf("Paciente nao encontrado.\n");
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
            // exige pelo menos um procedimento no historico
            if (ptree_history_size_by_id(pt, id) < 1) {
                message_and_clear("Alta proibida: historico vazio. Registre ao menos um procedimento.", MSG_WAIT_SHORT);
                continue;
            }

            /* Registra alta no historico e atualiza flag 'called' */
            {
                char item[PROC_MAX_LEN + 1];
                char timestr[32] = {0};

                (void)format_timestamp(timestr, sizeof(timestr));

                if (timestr[0] != '\0')
                    snprintf(item, sizeof(item), "[%s] Alta concedida", timestr);

                else { // tenta registrar alta no histórico e ignora erro se cheio
                    strncpy(item, "Alta concedida", sizeof(item) - 1);
                    item[sizeof(item) - 1] = '\0';
                }

                (void)ptree_history_push(pt, id, item);
            }

            (void)ptree_set_called(pt, id, false);
            (void)ptree_set_discharged(pt, id, true); /* marcar alta persistente */
            
            /* salvar estado após alta */
            (void)io_save(DATA_FILE, pt, q);
            message_and_clear("Alta concedida. Registro mantido. Retornando ao menu...", MSG_WAIT_MEDIUM);

        } 
        
        else if (opc == 8) {
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
                    util_printf("Dados salvos em %s. Até breve.\n", DATA_FILE);
                else
                    util_printf("Erro ao salvar dados.\n");
            } 
            
            else
                util_printf("Nenhum dado para salvar. Arquivo não foi alterado.\n");

            break;

        } 
        
        else {
            util_printf("Opção inválida.\n");
            message_and_clear("Retornando ao menu...", MSG_WAIT_SHORT);
        }
    }

    /* cleanup final: usar árvore + fila */
    ptree_destroy(pt);
    pqueue_destroy(q);

    return 0;
}