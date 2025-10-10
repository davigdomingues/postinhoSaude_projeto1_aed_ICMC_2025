@echo off
REM scripts\build.bat - compila o projeto a partir da raiz (gcc deve estar no PATH)

REM Muda para a pasta raiz do projeto (uma pasta acima de scripts)
pushd "%~dp0\.."

REM Garante que a pasta de saída existe
if not exist "output" mkdir output

REM Compilar todos os .c (C11, warnings, debug)
gcc -std=c11 -Wall -Wextra -g3 *.c -o output\main.exe
if %ERRORLEVEL% neq 0 (
    echo Erro: compilacao falhou com codigo %ERRORLEVEL%.
    popd
    exit /b %ERRORLEVEL%
)

echo Build concluido: output\main.exe

popd
exit /b 0
