#!/usr/bin/env bash
# Script para converter data.bin (assumindo CP1252) para UTF-8 (data.utf8)
# Uso: ./scripts/convert_data.sh

SRC="$(dirname "$0")/../data.bin"
DST="$(dirname "$0")/../data.utf8"

if [ ! -f "$SRC" ]; then
  echo "Arquivo $SRC nao encontrado."
  exit 2
fi

if command -v iconv >/dev/null 2>&1; then
  echo "Usando iconv para converter CP1252 -> UTF-8..."
  iconv -f CP1252 -t UTF-8 "$SRC" -o "$DST" && {
    echo "Convertido para $DST"
    exit 0
  } || {
    echo "Falha ao usar iconv."
  }
fi

if command -v python3 >/dev/null 2>&1; then
  echo "Usando python3 para converter (fallback)..."
  python3 - <<'PY' "$SRC" "$DST"
import sys
src, dst = sys.argv[1], sys.argv[2]
with open(src, "rb") as f:
    data = f.read()
# decodifica como cp1252 e re-encoda em utf-8
try:
    text = data.decode("cp1252")
except Exception as e:
    print("Erro ao decodificar como cp1252:", e, file=sys.stderr)
    sys.exit(1)
with open(dst, "wb") as f:
    f.write(text.encode("utf-8"))
print("Convertido para", dst)
PY
  exit $?
fi

echo "Nenhum conversor disponivel (iconv ou python3). Instale um e tente novamente."
exit 1
