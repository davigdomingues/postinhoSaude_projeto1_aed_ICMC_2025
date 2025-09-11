// clear_screen.c
// Funções utilitárias para limpar o terminal e exibir uma mensagem breve antes
// de limpar. Compatível com Windows e sistemas Unix-like.

#include "clear_screen.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
// Windows API -> Sleep()
#include <windows.h>
#else
// POSIX -> usleep()
#include <unistd.h> // Biblioteca POSIX para usleep(), que suspende a execução por microsegundos
#endif

void clear_screen(void) {
#ifdef _WIN32
    /* No Windows executamos o comando 'cls' no shell para limpar a tela. */
    system("cls");
#else
    /* Em sistemas Unix-like usamos 'clear'., onde system() invoca o shell. */
    system("clear");
#endif
}

/*
 * Mostra uma mensagem de confirmação, espera pelo número de milissegundos
 * especificado e limpa a tela (chama clear_screen()).
 *
 * Comportamento importante:
 * - fflush(stdout) é chamado para garantir que a mensagem apareça antes da pausa.
 * - No Windows usamos Sleep(milliseconds) (milissegundos).
 * - Em POSIX usamos usleep(microsegundos) por isso multiplicamos por 1000.
 * - Cuidado com valores muito grandes para 'milliseconds' (pode causar overflow ao multiplicar).
 */
void message_and_clear(const char *message, unsigned int milliseconds) {
    if (message && message[0] != '\0') {
        printf("%s\n", message);
    }

    /* Garante que a saída foi enviada ao terminal antes de dormir */
    fflush(stdout);

#ifdef _WIN32
    /* recebe milissegundos no Windows */
    Sleep(milliseconds);
#else
    /* usleep recebe microsegundos em POSIX */
    /* Multiplicamos por 1000 para converter ms -> us */
    usleep((useconds_t)milliseconds * 1000u);
#endif
    /* Após a pausa, limpa a tela para voltar ao menu principal */
    clear_screen();
}