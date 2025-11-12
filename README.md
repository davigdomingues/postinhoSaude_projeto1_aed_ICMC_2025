# Postinho de Saúde - Projeto 1 e 2 (AED, ICMC 2025)

## Resumo

Este repositório contém uma implementação didática de um sistema de gestão para um "postinho de saúde" (projeto didático de AED). Ele gere um cadastro de pacientes, uma fila de espera para atendimento e um histórico de procedimentos por paciente, com persistência em disco entre execuções.

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
6. Mostrar fila de espera (o programa resolve nomes e imprime; a fila não possui função de impressão pública).  
7. Mostrar histórico de um paciente.  
8. Sair e salvar os dados.

## Estrutura geral do código

Arquivos principais (esperados neste diretório):

- main.c              — ponto de entrada e interface com o usuário.
- config.h            — configurações e constantes (tamanhos máximos, capacidade da fila, nome do arquivo de dados).
- patient_list.h/c    — implementação da lista de pacientes (inserção, busca, remoção, liberação).
- queue.h/c           — implementação da fila de espera (enqueue, dequeue, contains, size, get_by_index; sem funções de UI).
- history.h/c         — pilha de procedimentos por paciente (push, pop, is_full).
- io.h/c              — funções de leitura/gravação para persistência (io_load, io_save).
- util.h/c            — utilitários (por exemplo, read_line).
- clear_screen.h/c    — função para limpar a tela em cada iteração do menu.
- proj1VersaoAtualizada.pdf — especificação do projeto.

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
  3. tamanho da fila (int)
  4. ids da fila (uma por linha)
- Observação: comprimentos das strings obedecem a `MAX_ID_LEN`, `MAX_NAME_LEN`, `PROC_MAX_LEN` em `config.h`. A especificação detalhada e regras de leitura estão em `io.h`.

## Compilação

Recomendado (MSYS/MinGW, Unix):

```bash
make
```

Executável produzido: `output/main.exe`

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
  - mkdir output
  - gcc -std=c11 -Wall -Wextra -g3 *.c -o output\main.exe
- Nota: este repositório inclui `scripts\build.bat` (script de compilação). Caso queira manter apenas o script dentro da pasta `scripts`, remova o `build.bat` na raiz (por exemplo: `git rm build.bat` e commite). Após remover o stub, atualize a task do VSCode (.vscode/tasks.json) para apontar para `scripts\\build.bat` se necessário.

(script incluído gera `output\main.exe`).

- Alternativa direta:

```bash
gcc -Wall -Wextra -g3 *.c -o output/main.exe
```

## Execução e teste rápido

1. (Opcional) Converter `data.bin` para UTF‑8 com `scripts/convert_data.*` se suspeitar de CP1252.
2. Abrir PowerShell/terminal e, se no Windows, executar os comandos de encoding indicados acima.
3. Executar:
   - Windows: `output\main.exe`
   - Unix/Git Bash: `./output/main.exe`
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
  - patient_list.c: adição de tabela hash interna (separate chaining) para mapear id→índice (plist_htable_*). plist_find_index usa a hash para O(1) em consultas habituais.
  - queue.c: adição de hash‑set interno para membership (qset_*), tornando queue_contains O(1).
  - Motivo: reduzir custo de validações/entities frequentes (plist_find_index, queue_contains) sem alterar formato de persistência.

- API/semântica mantidas
  - Não houve mudança nas assinaturas públicas (patient_list.h, queue.h, history.h, io.h permanecem compatíveis). Persistência (formato textual) aceita o mesmo layout; io_save continua gravando UTF‑8.

- Arquivos alterados (resumo)
  - io.c (normalização CP1252→UTF‑8 na carga)
  - io.h (documentação do formato permanece, mas io.c adicionou comentários/funcs internas)
  - util.c / util.h (util_printf, print_utf8)
  - patient_list.c / patient_list.h (tabela hash interna, ajustes)
  - queue.c / queue.h (hash‑set membership para fila, ajuste de prints)
  - history.c / history.h (uso de util_printf em debug)
  - main.c (substituição de prints que exibem dados carregados por util_printf)
  - README.md (esta atualização)

## Testes recomendados

1. Converter data.bin para UTF‑8 (opcional) e executar o programa no PowerShell após aplicar os comandos acima.
2. Verificar que nomes com acento são mostrados corretamente e que a fila/historico exibem entradas com acentos.
3. Inserir novos pacientes com acentos, salvar (opção 8) e reler o ficheiro para garantir persistência correta.

## Notas finais

- Documentação do formato e comportamentos de I/O está em `io.h` e implementação em `io.c`.
- Se desejar, pode-se estender `io_save` com backups rotativos, compressão ou encriptação — atualizar README e `io.h` se isso for implementado.
- Contribuições são bem-vindas. Abra issues ou pull requests com melhorias, correções de bugs ou documentação adicional.

## Notas adicionais (adicionadas)

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

Davi Gabriel Domingues (15447497)
Caio Cerceau Nanni (16858556)
Felipe Gausmann Socolowski (16812461)