# Postinho de Saúde — Projeto 1 (AED, ICMC 2025)

## Resumo

Este repositório contém a implementação de um sistema simples de gerenciamento para um "postinho de saúde" (projeto didático de AED). O programa gerencia um cadastro de pacientes, uma fila de espera para atendimento e um histórico de procedimentos por paciente. Também há persistência dos dados entre execuções.

## Contexto do PDF

O arquivo `proj1VersaoAtualizada.pdf` acompanhado neste repositório traz a especificação do projeto, requisitos e detalhes de implementação. Em linhas gerais, o sistema implementado segue o escopo descrito no PDF:

- Gerir uma lista de pacientes (cadastro, remoção e busca).
- Manter uma fila de espera (enfileirar, desenfileirar, imprimir, remover específico).
- Registrar um histórico de procedimentos por paciente (push/pop, limite de tamanho).
- Interface de linha de comando com menu para operações básicas (registro, alta/óbito, adicionar/desfazer procedimento, chamar próximo, listar fila, exibir histórico, salvar e sair).
- Persistência dos dados em arquivo (salvar/carregar ao iniciar/sair).

## Resumo rápido das funcionalidades (conforme implementação em `main.c`)

1. Registrar paciente (inserir no cadastro e opcionalmente na fila de espera).
2. Registrar óbito (remoção definitiva do cadastro com checagens de integridade).
3. Adicionar procedimento ao histórico do paciente.
4. Desfazer o último procedimento do histórico.
5. Chamar paciente (retirar da fila e marcar como chamado).
6. Mostrar fila de espera.
7. Mostrar histórico de um paciente.
8. Sair e salvar os dados.

## Estrutura geral do código

Arquivos principais (esperados neste diretório):

- main.c              — ponto de entrada e interface com o usuário.
- config.h            — configurações e constantes (tamanhos máximos, capacidade da fila, nome do arquivo de dados).
- patient_list.h/c    — implementação da lista de pacientes (inserção, busca, remoção, liberação).
- queue.h/c           — implementação da fila de espera (enqueue, dequeue, contains, remove, print, init/free).
- history.h/c         — pilha de procedimentos por paciente (push, pop, is_full).
- io.h/c              — funções de leitura/gravação para persistência (io_load, io_save).
- util.h/c            — utilitários (por exemplo, read_line).
- clear_screen.h/c    — função para limpar a tela em cada iteração do menu.
- proj1VersaoAtualizada.pdf — especificação do projeto.

## Compilação

Você pode compilar os arquivos com gcc ou usar um Makefile (se presente). Um exemplo com gcc:

```bash
gcc -std=c11 -Wall -Wextra -o postinho main.c patient_list.c queue.c history.c io.c util.c clear_screen.c
```

(Substitua/adicione outros arquivos .c conforme a organização real do repositório.)

## Execução

Após compilar, rode:

```bash
./postinho
```

O programa apresenta um menu em linha de comando onde o usuário pode escolher as opções descritas acima. Ao sair, os dados são salvos no arquivo definido em `config.h`.

## Notas de implementação

- O arquivo `main.c` já contém comentários explicativos sobre o fluxo principal, validações e uso dos módulos.
- A maior parte da lógica de manipulação de memória e estrutura de dados está modularizada nos headers/implementações listados acima.
- Recomenda-se revisar os retornos das funções de I/O para tratamento de erros mais robusto em produção.

## Contribuição

Contribuições são bem-vindas. Abra issues ou pull requests com melhorias, correções de bugs ou documentação adicional.