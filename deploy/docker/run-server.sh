#!/usr/bin/env bash
set -euo pipefail

SERVER="$1"
NEEDS_DB="${2:-no}"
PREDELAY="${3:-0}"

cd /app/emu/Output

if [ "$PREDELAY" != "0" ]; then
    sleep "$PREDELAY"
fi

if [ "$NEEDS_DB" = "yes" ]; then
    echo "[${SERVER}] waiting for database..."
    for _ in $(seq 1 180); do
        [ -f /run/db_ready ] && break
        sleep 1
    done
fi

echo "[${SERVER}] starting in $(pwd)"
exec "./${SERVER}.elf"
