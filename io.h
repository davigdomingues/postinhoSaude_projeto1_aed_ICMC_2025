/* Explicação:
 * - io_save(const char *path, const PatientList *pl, const Queue *q)
 *     Abre o ficheiro em modo escrita e grava:
 *       1) quantidade de pacientes (pl->size) em linha própria ("%zu\n"); (%zu é para size_t)
 *       2) para cada paciente: id, name e número de entradas do histórico (p->hist.top+1);
 *          depois grava cada entrada do histórico em linhas separadas;
 *       3) tamanho da fila q->size; em seguida os ids enfileirados respeitando a
 *          ordem circular a partir de q->head.
 *     Retorna 0 em sucesso, -1 se não conseguir abrir o ficheiro.
 *
 * - io_load(const char *path, PatientList *pl, Queue *q)
 *     Abre o ficheiro em modo leitura e reconstrói as estruturas na mesma ordem
 *     escrita por io_save. Para cada paciente lido, insere o cadastro via plist_insert
 *     e depois repõe o histórico via history_push. Após carregar pacientes, lê
 *     a lista de IDs da fila e chama queue_enqueue para reconstruir a fila.
 *     Retornos: 0 em sucesso, -1 se não conseguir abrir o ficheiro, -2 em erros de
 *     formato/leitura ou falha ao inserir dados nas estruturas.
 */

#ifndef IO_H
#define IO_H
#include "patient_list.h"
#include "queue.h"

/*
 * io_save/io_load formato (texto):
 * 1) number_of_patients (size_t) em sua própria linha
 * 2) para cada paciente:
 *    id (linha)
 *    name (linha)
 *    n_history (int) (linha)
 *    entradas do histórico (n_history linhas)
 *    called_flag (0/1) (linha)
 * 3) tamanho da fila (int) (linha)
 * 4) ids da fila (uma por linha, na ordem)
 */

int io_save(const char *path, const PatientList *pl, const Queue *q); // 0 ok, -1 erro
int io_load(const char *path, PatientList *pl, Queue *q); // 0 ok, -1 sem arquivo, -2 erro

#endif