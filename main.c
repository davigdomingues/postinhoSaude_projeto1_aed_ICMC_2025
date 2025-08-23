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
#include "config.h"
#include "patient_list.h" // header da lista de pacientes
#include "queue.h" // header da fila
#include "history.h" // header do histórico
#include "io.h" // header para salvar/carregar
#include "util.h" // header com read_line()

// Estruturas globais
PatientList pl; // Lista de pacientes
Queue q; // Fila de espera

int main(void) {
    int opc = 0;
    char buf[256];

    // Inicializa lista e fila
    plist_init(&pl);
    queue_init(&q, WAIT_CAP);

    for(;;) {
        printf("\nMenu:\n");
        printf("1. Registrar paciente\n");
        printf("2. Dar alta ao paciente\n");
        printf("3. Adicionar procedimento ao histórico\n");
        printf("4. Desfazer último procedimento\n");
        printf("5. Chamar próximo para atendimento\n");
        printf("6. Mostrar fila de espera\n");
        printf("7. Mostrar histórico do paciente\n");
        printf("8. Sair\n");
        printf("Escolha: ");

        if (!fgets(buf, sizeof(buf), stdin)) 
            break;
        
        opc = atoi(buf);

        if (opc == 1) {
            char id[MAX_ID_LEN + 1], name[MAX_NAME_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));

            if (plist_find_index(&pl, id) >= 0)
                printf("ID já registrado. Usando cadastro existente.\n");
            
            else {
                printf("Nome: "); read_line(name, sizeof(name));
                int r = plist_insert(&pl, id, name);
                if (r == 0) printf("Paciente cadastrado.\n");
                else { printf("Falha ao cadastrar.\n"); continue; }
            }

            if (queue_is_full(&q))
                printf("Fila cheia. Não foi possível inserir.\n");

            else if (queue_contains(&q, id))
                printf("Paciente já está na fila de espera.\n");

            else {
                queue_enqueue(&q, id);
                printf("Paciente inserido na fila.\n");
            }

        } else if (opc == 2) {
            char id[MAX_ID_LEN + 1];
            printf("ID para alta: "); read_line(id, sizeof(id));

            if (queue_remove(&q, id) == 0)
                printf("Alta realizada. Removido da fila.\n");
            
            else
                printf("Paciente não está na fila (ou não existe).\n");

        } else if (opc == 3) {
            char id[MAX_ID_LEN + 1], proc[PROC_MAX_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));
            Patient *p = plist_get(&pl, id);

            if (!p) { 
                printf("Paciente não encontrado.\n"); 
                continue; 
            }

            if (history_is_full(&p->hist)) { 
                printf("Histórico cheio.\n"); 
                continue; 
            }

            printf("Procedimento (até %d chars): ", PROC_MAX_LEN);
            read_line(proc, sizeof(proc));

            if (history_push(&p->hist, proc) == 0)
                printf("Procedimento adicionado.\n");
            
            else
                printf("Falha ao adicionar procedimento.\n");

        } else if(opc == 4) {
            char id[MAX_ID_LEN + 1], out[PROC_MAX_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));
            Patient *p = plist_get(&pl, id);

            if (!p) { 
                printf("Paciente não encontrado.\n"); 
                continue; 
            }

            if (history_pop(&p->hist, out, sizeof(out)) == 0)
                printf("Procedimento desfeito: %s\n", out);

            else
                printf("Não há procedimento a desfazer.\n");

        } else if(opc == 5) {
            char id[MAX_ID_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));
            if(queue_dequeue(&q, id, sizeof(id)) == 0) {
                Patient *p = plist_get(&pl, id);
                printf("Chamando: ID %s | Nome: %s\n", id, p ? p->name : "(desconhecido)");
            } 
            
            else
                printf("Fila vazia.\n");

        } else if(opc == 6) {
            queue_print(&q);

        } else if(opc == 7) {
            char id[MAX_ID_LEN + 1];
            printf("ID: "); read_line(id, sizeof(id));
            Patient *p = plist_get(&pl, id);

            if(!p) { 
                printf("Paciente não encontrado.\n"); 
                continue; 
            }
            
            printf("Histórico de %s (ID %s): %d item(ns)\n",
                   p->name, p->id, p->hist.top + 1);

            for(int i = 0; i <= p->hist.top; i++)
                printf("%d) %s\n", i + 1, p->hist.items[i]);

        } else if(opc == 8) {
            if(io_save(DATA_FILE, &pl, &q) == 0)
                printf("Dados salvos em %s. Até breve!\n", DATA_FILE);

            else
                printf("Erro ao salvar dados.\n");

            break;

        } 
        
        else
            printf("Opção inválida.\n");
    }

    queue_free(&q);
    plist_free(&pl);

    return 0;
}