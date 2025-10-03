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

## Persistência (DATA_FILE)

- Ficheiro usado: definido em `config.h` como `DATA_FILE` (padrão: `data.bin`).
- Comportamento de gravação:
  - `io_save` escreve num ficheiro temporário (`data.bin.tmp`) e só renomeia para o ficheiro final após sucesso de escrita/fecho; isto reduz o risco de corromper o ficheiro persistente.
  - O programa evita sobrescrever `DATA_FILE` com estado vazio quando a carga inicial falhou e não houver dados novos na sessão.
- Formato do ficheiro (texto, linha a linha):
  1. número de pacientes (unsigned long)
  2. para cada paciente:
     - id (linha)
     - name (linha)
     - n_history (int)
     - n_history linhas com entradas do histórico
     - called_flag (0/1)
  3. tamanho da fila (int)
  4. ids da fila (uma por linha)
- Observação: comprimentos das strings obedecem a `MAX_ID_LEN`, `MAX_NAME_LEN`, `PROC_MAX_LEN` em `config.h`. A especificação detalhada e regras de leitura estão em `io.h`.

## Compilação

- Com Make (recomendado, MSYS/MinGW): na raiz do projeto:

```bash
make
```

Executável: `output/main.exe`.

- No Windows sem make:

```bash
build.bat
```

(script incluído gera `output\main.exe`).

- Alternativa direta:

```bash
gcc -Wall -Wextra -g3 *.c -o output/main.exe
```

## Execução

- No terminal:
  - Windows: `output\main.exe`
  - Git Bash / Unix: `./output/main.exe`
- Fluxo básico:
  - Ao iniciar, o programa tenta carregar `DATA_FILE` (se válido mostrará resumo e históricos).
  - Interaja via menu. Ao sair (opção 8) os dados são gravados, se houver estados válidos para salvar.

## Depuração de problemas de persistência

- Se ao sair o `data.bin` fica com zeros ou é substituído por um arquivo vazio:
  - Verifique mensagens exibidas ao iniciar e ao sair (erros de `io_load`/`io_save`).
  - Verifique permissões do ficheiro e presença de `data.bin.tmp` no diretório.
  - Execute o programa diretamente no terminal (não em background/IDE) para ver a saída completa.
  - Para inspecionar `data.bin`:
    - Windows: `type data.bin`
    - Git Bash: `cat data.bin`
- Se `io_load` retornar erro de formato (-2), o ficheiro pode estar corrompido ou não seguir o formato esperado; consulte `io.h` para o formato exato.
- Se ocorrerem "undefined reference" durante build, certifique-se de compilar/linkar todos os .c do projeto (Makefile ou `gcc *.c`).

## VSCode (dica rápida)

- Task de build já configurada para chamar `build.bat`. Use Ctrl+Shift+B para compilar.
- Se preferir terminal Git Bash integrado: Ctrl+Shift+P → "Terminal: Select Default Profile" → "Git Bash".
- Para depurar crie/ajuste `.vscode/launch.json` apontando para `output/main.exe`.

## Notas finais

- Documentação do formato e comportamentos de I/O está em `io.h` e implementação em `io.c`.
- Se desejar, pode-se estender `io_save` com backups rotativos, compressão ou encriptação — atualizar README e `io.h` se isso for implementado.
- Contribuições são bem-vindas. Abra issues ou pull requests com melhorias, correções de bugs ou documentação adicional.
