param(
  [string]$CC = "gcc",
  [string]$CFLAGS = "-std=c11 -Wall -Wextra -g3"
)

# root = pasta do projeto
$root = (Get-Item (Join-Path $PSScriptRoot "..")).FullName
$output = Join-Path $root "output"
if (-not (Test-Path $output)) { New-Item -ItemType Directory -Path $output | Out-Null }

Push-Location $root
$cmd = "$CC $CFLAGS *.c -o output\main.exe"
Write-Host "Executing: $cmd"
# executar via cmd para lidar com wildcard *.c de forma simples
$proc = Start-Process -FilePath "cmd.exe" -ArgumentList "/c $cmd" -NoNewWindow -Wait -PassThru
$rc = $proc.ExitCode
Pop-Location

if ($rc -ne 0) { Write-Error "Build failed (exit $rc)"; exit $rc }

Write-Host "Built: $output\main.exe"
