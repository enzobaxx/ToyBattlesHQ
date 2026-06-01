#!/usr/bin/env bash
set -euo pipefail

export MV_DB_PORT="${MV_DB_PORT:-3305}"

mkdir -p /run/mysqld
chown -R mysql:mysql /run/mysqld

if [ ! -d /var/lib/mysql/mysql ]; then
    echo "[entrypoint] Initializing MariaDB data directory..."
    mariadb-install-db --user=mysql --datadir=/var/lib/mysql --auth-root-authentication-method=normal >/dev/null 2>&1
fi
chown -R mysql:mysql /var/lib/mysql

echo "[entrypoint] Launching services (MariaDB on 127.0.0.1:${MV_DB_PORT})..."
exec supervisord -c /app/supervisord.conf
