/* Explicação:
 * - io_save(const char *path, const PatientList *pl, const Queue *q)
 *     Persiste as estruturas em disco. Modo de escrita (resumo dos passos):
 *       1) Cria um caminho temporário (path + ".tmp").
 *       2) Abre o ficheiro temporário em modo texto para escrita.
 *       3) Escreve o conteúdo em linhas com fprintf/fputs (formato textual documentado abaixo).
 *       4) Fecha o ficheiro temporário (fclose). Se fclose falhar, apaga o temporário e retorna erro.
 *       5) Substitui o ficheiro alvo: remove(path) (ignora erro se não existir) e rename(tmp, path).
 *          Assim evita-se deixar o ficheiro alvo em estado parcial (comportamento atômico simples).
 *     Observações importantes:
 *       - Não é usado locking de ficheiro neste módulo; se for necessário concorrência, acrescente lock externo.
 *       - io_save retorna 0 em sucesso e -1 em caso de erro de I/O (criação/ escrita / rename).
 *
 * - io_load(const char *path, PatientList *pl, Queue *q)
 *     Lê e reconstrói as estruturas a partir do ficheiro em 'path' seguindo exatamente o formato
 *     escrito por io_save. Procede com validações simples (fgets/fscanf + chomp); se linhas faltarem
 *     ou o formato não corresponder, aborta e retorna -2 (erro de formato/leitura). Se o ficheiro não
 *     existir ou não puder ser aberto retorna -1.
 *
 * Formato textual detalhado (linha por linha):
 * 1) number_of_patients (unsigned long) em sua própria linha
 * 2) para cada paciente (repetir number_of_patients vezes):
 *    a) id (linha)                      -- string terminada por '\n'
 *    b) name (linha)                    -- string terminada por '\n'
 *    c) n_history (int) (linha)         -- número de entradas do histórico
 *    d) entradas do histórico (n_history linhas)
 *    e) called_flag (0/1) (linha)       -- se foi chamado (1) ou não (0)
 * 3) tamanho da fila (int) (linha)      -- número de IDs que seguem
 * 4) ids da fila (uma por linha, na ordem do atendimento)
 *
 * Regras/limitações do formato:
 * - Cada linha termina com '\n'; entradas podem conter espaços e colchetes (sem escapamento).
 * - Comprimentos máximos das strings respeitam MAX_ID_LEN, MAX_NAME_LEN, PROC_MAX_LEN (config.h).
 * - A leitura é conservadora: entradas truncadas ou linhas em falta resultam em retorno -2.
 *
 * Códigos de retorno (resumido):
 *  - io_save: 0 -> sucesso, -1 -> erro de I/O (criar/escrever/fechar/renomear)
 *  - io_load: 0 -> sucesso, -1 -> ficheiro não existe / não pode ser aberto, -2 -> erro de formato/leitura
 *
 * Observações de uso:
 * - io_save usa um ficheiro temporário e rename para reduzir risco de corrupção do ficheiro persistente.
 * - Se precisar de políticas diferentes (por exemplo backups rotativos, compressão ou criptografia),
 *   modifique io_save/io_load e ajuste o formato/documentação aqui.
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

/*
 * Observação sobre encoding:
 * - io_load() realiza uma normalização simples das linhas lidas:
 *     * se a linha for válida UTF-8, é usada tal como está;
 *     * se a linha não for UTF-8 válida, io_load assume que está em CP1252
 *       (Windows-1252) e tenta convertê-la para UTF-8 antes de armazenar em memória.
 * - Isto permite compatibilidade com ficheiros de dados antigos gerados em ambientes
 *   Windows que utilizem CP1252. É possível forçar uma conversão externa,
 *   configurando o ficheiro para UTF-8 (ex.: iconv ou PowerShell) antes de executar.
 */

int io_save(const char *path, const PatientList *pl, const Queue *q); // 0 ok, -1 erro
int io_load(const char *path, PatientList *pl, Queue *q); // 0 ok, -1 sem arquivo, -2 erro

#endif