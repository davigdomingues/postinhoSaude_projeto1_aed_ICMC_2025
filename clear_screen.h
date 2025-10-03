#ifndef CLEAR_SCREEN_H
#define CLEAR_SCREEN_H

/* Limpa a tela do terminal, usando "cls" no Windows e "clear" em sistemas Unix.
 *
 * Observação:
 * - Funções aqui controlam a aparência no terminal (pausas e limpeza) e não têm
 *   impacto na persistência de dados.
 */
void clear_screen(void);

/*
 * Mostra uma mensagem de confirmação, espera pelo número de milissegundos
 * especificado e limpa a tela.
 */
void message_and_clear(const char *message, unsigned int milliseconds);

#endif