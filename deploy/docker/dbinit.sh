#!/usr/bin/env bash
set -euo pipefail

DB_PORT="${MV_DB_PORT:-3305}"
DB_PW="${MV_DB_PW:?MV_DB_PW must be set}"
SOCK=/run/mysqld/mysqld.sock

echo "[dbinit] Waiting for MariaDB socket..."
for _ in $(seq 1 90); do
    if mariadb-admin --socket="$SOCK" ping >/dev/null 2>&1; then
        break
    fi
    sleep 1
done

NOPW=(mariadb --socket="$SOCK" -u root)
PW=(mariadb --socket="$SOCK" -u root "-p${DB_PW}")

if "${NOPW[@]}" -e "SELECT 1" >/dev/null 2>&1; then
    CLIENT=("${NOPW[@]}")
else
    CLIENT=("${PW[@]}")
fi

if "${CLIENT[@]}" -e "USE \`microvolts-db\`; SELECT 1 FROM Users LIMIT 1;" >/dev/null 2>&1; then
    echo "[dbinit] Database already initialized."
else
    echo "[dbinit] Creating database 'microvolts-db'..."
    "${CLIENT[@]}" -e "CREATE DATABASE IF NOT EXISTS \`microvolts-db\`;"
    echo "[dbinit] Importing schema and seed data..."
    "${CLIENT[@]}" microvolts-db < /app/emu/microvolts-db.sql
    echo "[dbinit] Setting root password and TCP access..."
    "${CLIENT[@]}" <<SQL
ALTER USER 'root'@'localhost' IDENTIFIED BY '${DB_PW}';
CREATE USER IF NOT EXISTS 'root'@'127.0.0.1' IDENTIFIED BY '${DB_PW}';
GRANT ALL PRIVILEGES ON *.* TO 'root'@'127.0.0.1' WITH GRANT OPTION;
FLUSH PRIVILEGES;
SQL
    echo "[dbinit] Database ready."
fi

touch /run/db_ready
