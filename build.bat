REM Instruções rápidas:
REM - Abra o VSCode na pasta do projeto.
REM - Ctrl+Shift+P -> "Terminal: Select Default Profile" -> escolha "Git Bash".
REM - Terminal -> New Terminal para abrir Git Bash integrado.
REM - Para executar este script no Git Bash: use "cmd.exe /c build.bat" ou execute "build.bat" se já estiver associado.

@echo off
REM Compila todos os .c do diretório e gera output\main.exe
if not exist output (
    mkdir output
)
gcc -Wall -Wextra -g3 *.c -o output\main.exe
if errorlevel 1 (
    echo Build falhou.
    exit /b 1
) else (
    echo Build concluído: output\main.exe
)
