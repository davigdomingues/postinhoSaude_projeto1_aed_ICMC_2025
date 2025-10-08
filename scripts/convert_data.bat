@echo off
REM Script para converter data.bin (assumindo CP1252) para UTF-8 (data.utf8)
REM Uso: abra PowerShell/Prompt no diretório do projeto e execute scripts\convert_data.bat

set SRC=%~dp0\..\data.bin
set DST=%~dp0\..\data.utf8

if exist "%SRC%" (
    echo Detectando iconv...
    where iconv >nul 2>nul
    if %ERRORLEVEL%==0 (
        echo Usando iconv para converter CP1252 -> UTF-8...
        iconv -f CP1252 -t UTF-8 "%SRC%" -o "%DST%" && (
            echo Convertido para "%DST%"
            exit /b 0
        ) || (
            echo Falha ao usar iconv.
        )
    )

    echo Usando PowerShell como fallback para converter (Default -> UTF8)...
    powershell -NoProfile -Command "Get-Content -Raw -Encoding Default '%SRC%' | Set-Content -Encoding UTF8 '%DST%'"
    if %ERRORLEVEL%==0 (
        echo Convertido para "%DST%" usando PowerShell.
        exit /b 0
    ) else (
        echo Erro: nao foi possivel converter o ficheiro.
        exit /b 1
    )
) else (
    echo Ficheiro "%SRC%" nao encontrado.
    exit /b 2
)
