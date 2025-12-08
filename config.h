#ifndef CONFIG_H
    #define CONFIG_H

    /*
    Este header define constantes de configuração usadas por todo o projeto.

    - MAX_ID_LEN  : comprimento máximo (em caracteres) do identificador de paciente.
                    Usado para dimensionar buffers que armazenam IDs (ex.: char id[MAX_ID_LEN + 1]).
    - MAX_NAME_LEN: comprimento máximo do nome do paciente (idem acima).
    - HIST_MAX     : número máximo de procedimentos armazenáveis no histórico de um paciente.
                    Usado pelo módulo history para definir o tamanho da pilha/array.
    - PROC_MAX_LEN : comprimento máximo (em caracteres) de uma descrição de procedimento.
    - WAIT_CAP     : capacidade máxima da fila de espera (quantos IDs podem estar enfileirados).

    Notas:
    - Os buffers no código principal adicionam +1 para o terminador nulo ('\0').
    - Ajustar estas constantes permite adaptar a aplicação a diferentes requisitos de armazenamento/limites.
    */

    #define MAX_ID_LEN 32
    #define MAX_NAME_LEN 80
    #define HIST_MAX 10
    #define PROC_MAX_LEN 300

    #define WAIT_CAP 50 // capacidade da fila de espera

    /* Nome do ficheiro usado por io_save / io_load. */
    #define DATA_FILE "bin/data.bin"

    /* Durações de tempo para message_and_clear (em milissegundos). */
    #define MSG_WAIT_SHORT 3000U
    #define MSG_WAIT_MEDIUM 4000U
    #define MSG_WAIT_LONG 5000U

    /*
     * Observação:
     * - DATA_FILE aponta para o ficheiro usado por io_save/io_load; alterar este valor
     *   muda o ficheiro persistido por todo o sistema.
     */

    /*
     * Capacidades dinâmicas (tempo de execução):
     * - As macros HIST_MAX e WAIT_CAP continuam servindo como valores padrão.
     * - Em runtime, é possível alterar os limites de capacidade chamando:
     *     config_set_hist_max(int cap): define a capacidade padrão para novos históricos.
     *     config_set_wait_cap(int cap): define a capacidade padrão para novas filas de espera.
     * - Os getters retornam o valor corrente, caindo no padrão das macros se ainda não ajustado:
     *     config_get_hist_max(), config_get_wait_cap().
     * - Observações:
     *   * A capacidade é aplicada na criação das estruturas (history_create, pqueue_create).
     *   * Alterações posteriores não redimensionam estruturas já existentes.
     *   * Implementação dos setters/getters reside em util.c.
     */
    
    void config_set_hist_max(int cap);
    int  config_get_hist_max(void);

    void config_set_wait_cap(int cap);
    int  config_get_wait_cap(void);

#endif