<!-- markdownlint-disable MD037 -->
# Postinho de Saúde - Projeto 1 e 2 (AED, ICMC 2025)

## Resumo

Este repositório contém uma implementação didática de um sistema de gestão para um "postinho de saúde" (projeto didático de AED). Ele gere um cadastro de pacientes, uma fila de espera para atendimento e um histórico de procedimentos por paciente, com persistência em disco entre execuções.

## Contexto do Trabalho

Os arquivos presentes no diretório 'documentacaoEnunciado_projetos' (e materiais auxiliares) acompanhados neste repositório trazem a especificação do projeto em cada parte (1 e 2), os requisitos e os detalhes de implementação. Em linhas gerais, o sistema implementado segue o escopo descrito nos PDFs:

- Gerir uma lista de pacientes (cadastro, remoção e busca).
- Manter uma fila de espera (enfileirar, desenfileirar, imprimir, remover específico).
- Registrar um histórico de procedimentos por paciente (push/pop, limite de tamanho).
- Interface de linha de comando com menu para operações básicas (registro, alta/óbito, adicionar/desfazer procedimento, chamar próximo, listar fila, exibir histórico, salvar e sair).
- Persistência dos dados em arquivo (salvar/carregar ao iniciar/sair).

## Resumo rápido das funcionalidades (conforme implementação em `main.c`)

1. Registrar paciente (cadastra na árvore e opcionalmente insere na fila por prioridade).
2. Remover paciente (óbito) — só permite se não estiver na fila e se já tiver sido chamado.
3. Listar pacientes (inorder por ID).
4. Buscar paciente por ID (submenu):
   - 1: Adicionar procedimento (com timestamp, se disponível).
   - 2: Desfazer último procedimento.
   - 3: Mostrar histórico completo.
   - 4: Voltar.
5. Chamar próximo paciente (dequeue pela menor prioridade; desempate por chegada).
6. Mostrar fila de espera (resolve nomes por árvore e exibe prioridade).
7. Dar alta (remove registro se foi chamado e não está na fila).
8. Sair e salvar os dados.

## Estrutura geral do código

Arquivos principais (esperados neste diretório):

- main.c              — ponto de entrada e interface com o usuário.
- config.h            — configurações e constantes (tamanhos máximos, capacidade da fila, nome do arquivo de dados).
- patient_tree.h/c    — árvore AVL de pacientes (busca/insere/remove, flags e prioridade).
- priority_queue.h/c  — fila de prioridade (heap min por prioridade 1..5, desempate por chegada).
- history.h/c         — pilha de procedimentos por paciente (push, pop, is_full).
- io.h/c              — funções de leitura/gravação para persistência (io_load, io_save, normalização CP1252→UTF‑8).
- util.h/c            — utilitários (read_line, util_printf UTF‑8, format_timestamp, locale).
- clear_screen.h/c    — utilidades de UI (limpar tela e mensagens temporizadas).
- proj1VersaoAtualizada.pdf, projeto2.pdf — especificações do projeto (subdivido em duas partes).

## Persistência (DATA_FILE)

- Ficheiro usado: definido em `config.h` como `DATA_FILE` (padrão: `bin/data.bin`).
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
     - priority (int) — em ficheiros antigos pode estar ausente e assume 5
     - discharged_flag (0/1) — em ficheiros antigos pode estar ausente e assume 0
  3. tamanho da fila (int)
  4. para cada item da fila:
     - id (linha)
     - priority (int) — novo campo; em ficheiros antigos pode estar ausente e assume 5
- Observação: comprimentos das strings obedecem a `MAX_ID_LEN`, `MAX_NAME_LEN`, `PROC_MAX_LEN` em `config.h`. A especificação detalhada e regras de leitura estão em `io.h`.

## Compilação

Recomendado (MSYS/MinGW, Unix):

```bash
make
```

Executável produzido: `/main` (Linux) ou `main.exe` (Windows)

Alternativa (Windows sem make):

```bat
scripts\build.bat
```

Adicional (Windows / PowerShell)

- Se o comando `make` não existir no sistema (erro: 'make' não é reconhecido), use um dos scripts de build incluídos:
  - PowerShell: .\scripts\build.ps1
  - CMD/PowerShell: .\scripts\build.bat
- Alternativa rápida sem scripts (no PowerShell):
  - cd "d:\pastasGitClonadas\postinhoSaude_projeto1_aed_ICMC_2025"
  - gcc -std=c11 -Wall -Wextra -g3 *.c -o main.exe
- Nota: este repositório inclui `scripts\build.bat` (script de compilação). Caso queira manter apenas o script dentro da pasta `scripts`, remova o `build.bat` na raiz (por exemplo: `git rm build.bat` e commite). Após remover o stub, atualize a task do VSCode (.vscode/tasks.json) para apontar para `scripts\\build.bat` se necessário.

(script incluído gera `output\main.exe`).

- Alternativa direta:

```bash
gcc -Wall -Wextra -g3 *.c -o main
```

## Execução e teste rápido

1. (Opcional) Converter `data.bin` para UTF‑8 com `scripts/convert_data.*` se suspeitar de CP1252.
2. Abrir PowerShell/terminal e, se no Windows, executar os comandos de encoding indicados acima.
3. Executar:
   - Windows: `\main.exe`
   - Unix/Git Bash: `/main`
4. Teste: insira nomes com acentos, adicione histórico, salve (opção 8), reinicie e verifique a persistência/exibição.

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

## Encoding e execução no Windows

Observação importante: o projeto normaliza textos lidos do ficheiro de dados para UTF‑8 ao carregar (io_load) e usa um mecanismo robusto de escrita no console no Windows para garantir que acentos sejam exibidos corretamente no PowerShell/Windows Terminal.

- Por que foi necessário
  - Ficheiros de dados antigos podem conter texto em CP1252 (Windows‑1252) ou UTF‑8; misturas produziam "mojibake" (ex.: "SÃ£o" em vez de "São").
  - O programa detecta se uma linha NÃO é UTF‑8 válida e, nesse caso, converte CP1252→UTF‑8 ao carregar (io.c).
  - Para exibir UTF‑8 corretamente no Windows, o programa usa um wrapper de impressão que, no Windows, converte UTF‑8→UTF‑16 e escreve com WriteConsoleW (util.c). Em sistemas Unix-like o comportamento é o mesmo de antes (vprintf/fputs).

- Regras práticas para executar no PowerShell antes de rodar o programa
  - Na sessão atual execute:
    chcp 65001
    $OutputEncoding = [System.Text.UTF8Encoding]::new()
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    [Console]::InputEncoding  = [System.Text.Encoding]::UTF8
  - Recomendado: usar PowerShell 7 (pwsh) ou Windows Terminal — eles lidam com UTF‑8 com menos problemas.

- Conversão preventiva de dados
  - Se preferir garantir que o ficheiro de dados esteja todo em UTF‑8, converta com:
    iconv -f CP1252 -t UTF-8 data.bin > data.utf8
  - O programa já tenta normalizar automaticamente no carregamento, por isso a conversão manual é opcional.

## Notas de implementação (resumo)

- Normalização de encoding
  - io.c: io_load agora chama normalize_to_utf8_inplace() em ids, nomes, linhas do histórico e ids da fila; função cp1252_to_utf8 converte CP1252→UTF‑8 quando necessário.
  - Motivo: suportar ficheiros antigos em CP1252 e evitar mojibake ao exibir nomes com acentos.

- Saída UTF‑8 robusta no Windows
  - util.c / util.h: adicionadas util_printf() e print_utf8() que, no Windows, convertem UTF‑8→UTF‑16 e usam WriteConsoleW. Substituídos prints que exibem dados persistidos por util_printf para garantir exibição correta em PowerShell.
  - Motivo: garantir que printf mostre acentuação corretamente no conhost/PowerShell.

- Otimizações de desempenho
  - patient_tree.c: implementação de árvore AVL para operações O(log n), priorizando a busca de pacientes.
  - priority_queue.c: heap com hash-set interno para membership rápido, a fim de amortizar as operações da árvore em si.
  - Motivo: reduzir custo de validações/entities frequentes (plist_find_index, queue_contains) sem alterar formato de persistência.

- API/semântica mantidas (comparado ao projeto 1)
  - Não houve mudança nas assinaturas públicas (patient_tree.h, priority_queue.h, history.h, io.h permanecem compatíveis). Persistência (formato textual) aceita o mesmo layout; io_save continua gravando UTF‑8.

- Arquivos alterados, em relação ao projeto 1 (resumo)
  - io.c/io.h (prioridade por paciente e por item na fila no formato textual; normalização CP1252→UTF‑8)
  - util.c / util.h (util_printf, print_utf8, util_setup_locale)
  - patient_tree.c / patient_tree.h (AVL, wrappers de histórico, prioridade e called)
  - priority_queue.c / priority_queue.h (heap de prioridade + membership)
  - queue.c / queue.h e patient_list.c / .h (mantidos por compatibilidade e referência; não usados pelo main atual)
  - main.c (uso de ptree+pqueue; impressão UTF‑8)
  - README.md (esta atualização)

## Testes recomendados

1. Converter data.bin para UTF‑8 (opcional) e executar o programa no PowerShell após aplicar os comandos acima.
2. Verificar que nomes com acento são mostrados corretamente e que a fila/historico exibem entradas com acentos.
3. Inserir novos pacientes com acentos, salvar (opção 8) e reler o ficheiro para garantir persistência correta.

## Arquitetura atual (resumo rápido)

- Estruturas de runtime:
  - Árvore AVL de pacientes (patient_tree.*): busca/insere/remove em O(log n), histórico por paciente, flags 'called' e prioridade 1..5.
  - Fila de prioridade (priority_queue.*): heap min por prioridade (1 = emergência ... 5 = não urgência) com desempate por ordem de chegada (seq).
  - Histórico (history.*): pilha fixa por paciente (HIST_MAX).
  - I/O (io.*): formato textual com normalização UTF‑8 em carga (CP1252→UTF‑8 quando necessário) e escrita robusta via tmp+rename; leitura compatível com ficheiros antigos sem prioridade/alta.
  - Utilidades (util.* e clear_screen.*): leitura segura, timestamps, console UTF‑8 (Windows) e UI básica.

- Observação:
  - A fila simples (queue.*) e lista (patient_list.*) permanecem no repositório por compatibilidade e referência, mas o main usa patient_tree + priority_queue como implementação padrão (a versão "retrô" está presente em codigo/postinhoSUS/zipFinal_projeto1).

## Conformidade com C99

- Objetivo do projeto: código compatível com C99 e pedantic.
- Build padrão do Makefile está em C11 por conveniência. Para verificação C99:
  - `gcc -std=c99 -pedantic -Wall -Wextra -I. -c *.c`
- Pontos de atenção:
  - Comentários // são aceitos no C99.
  - Funções util_* substituem strnlen/strdup para evitar dependências fora do C99.
  - Thread‑safety: util_localtime usa cópia a partir de localtime() mantendo portabilidade C99.

## Verificação local (C99)

- Consulte `C99_REPORT.txt` para comandos e dicas de diagnóstico.
- Compilar sem link para inspecionar warnings:
  - gcc -std=c99 -pedantic -Wall -Wextra -I. -c *.c
- Opcional: pedantic strict
  - gcc -std=c99 -pedantic-errors -Wall -Wextra -I. -c *.c
- Em caso de erros:
  - Verifique includes (stdbool.h, stddef.h, string.h, time.h).
  - Revise conversões de tamanho (size_t ↔ int) e casts em snprintf/printf.
- Dica rápida (sem linkagem, apenas checagem pedantic):
  - gcc -std=c99 -pedantic -Wall -Wextra -I. -c *.c

## Testes/validação rápida

- Fluxo recomendado:
  1. Carregar dados com io_load e confirmar impressão correta de acentos.
  2. Registrar paciente com prioridade, adicionar procedimentos com timestamp, salvar e reiniciar.
  3. Testar chamada por prioridade e verificar ordem por prioridade e chegada (seq).
- Diagnóstico:
  - Inspecionar data.bin com type/cat e validar layout textual conforme io.h.
  - Se UTF‑8 quebrado, confirmar normalização automática no io_load ou converter com iconv.

## Dependências

- Compilador C (gcc ou clang) com suporte a C11 (Makefile) e opcionalmente C99 para verificação.
- Ferramentas opcionais:
  - make (para usar o Makefile)
  - PowerShell ou Git Bash (Windows) para comandos de encoding e execução
  - iconv (opcional) para conversão de dados CP1252→UTF‑8

## Notas finais

- Este README reflete a arquitetura com árvore AVL e fila de prioridade usadas por main.c atualmente.
- Módulos anteriores (patient_list/queue) ficam como referência e podem ser reutilizados se necessário ajustando main.c e io.*.
- Documentação do formato e comportamentos de I/O está em `io.h` e implementação em `io.c`.
- Se desejar, pode-se estender `io_save` com backups rotativos, compressão ou encriptação — atualizar README e `io.h` se isso for implementado.
- Contribuições são bem-vindas. Abra issues ou pull requests com melhorias, correções de bugs ou documentação adicional.

## Notas adicionais

- Ficheiro sample `data.bin` incluído:
  - O `data.bin` presente no repositório é um exemplo de ficheiro de persistência com entradas de teste (nomes com acentos, históricos e fila).
  - Espera‑se que o ficheiro esteja em UTF‑8. Se observar "mojibake" (texto com caracteres estranhos), use:
    - Unix: iconv -f CP1252 -t UTF-8 data.bin > data.utf8 && mv data.utf8 data.bin
    - Windows/PowerShell: convertendo via ferramentas externas ou abrir no editor e salvar em UTF‑8.
  - Para inspecionar rapidamente: `head -n 50 data.bin` (Git Bash / Unix) ou `type data.bin` (PowerShell/cmd).

- Testes rápidos / verificação do build:
  - Compilar com gcc (conforme README):
    - make
  - Alternativa com clang:
    - clang -std=c11 -Wall -Wextra -g3 *.c -o output/main.exe
  - Executar e percorrer opções do menu para validar:
    1. Registrar 1–2 pacientes com nomes acentuados;
    2. Adicionar procedimentos e salvar (opção 8);
    3. Reiniciar o programa e verificar que os dados recarregam corretamente.

- Problemas conhecidos e dicas:
  - Windows console:
    - Para exibir corretamente UTF‑8 no cmd/PowerShell, executar antes de rodar:
      chcp 65001
      $OutputEncoding = [System.Text.UTF8Encoding]::new()
      [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    - PowerShell 7 / Windows Terminal recomenda‑se por melhor suporte a UTF‑8.
  - Permissões de ficheiro:
    - Se `io_save` falhar ao renomear, verifique se `data.bin` não está aberto por outro processo e se tem permissões de escrita no diretório.
  - Backup:
    - `io_save` grava num temporário e depois renomeia; se surgirem ficheiros `.tmp` no diretório após falhas, pode removê‑los manualmente antes de reexecutar.

- Contribuições e estilo:
  - Pequenas alterações (docs, correções simples) são bem‑vindas.
  - Para código novo ou refatorações maiores:
    - Mantenha convenções do projeto (C11, checks de retorno, buffers terminados com '\0').
    - Teste em ambos ambientes (Unix / Windows) sempre que possível.
    - Abra um issue com descrição curta do problema e passos para reproduzir antes de submeter pull request.

## Autores (nome - número USP)

- Davi Gabriel Domingues (15447497)
- Caio Cerceau Nanni (16858556)
- Felipe Gausmann Socolowski (16812461)
