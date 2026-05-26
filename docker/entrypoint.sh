#!/usr/bin/env bash
set -euo pipefail

DB_HOST="${DB_HOST:-db}"
DB_PORT="${DB_PORT:-3306}"
DB_USER="${DB_USER:-mangos}"
DB_PASSWORD="${DB_PASSWORD:-mangos}"
DB_WAIT_TIMEOUT="${DB_WAIT_TIMEOUT:-180}"

MANGOS_CONF="/opt/turtlewow/server/etc/mangosd.conf"
MANGOS_DIST="/opt/turtlewow/server/etc/mangosd.conf.dist"
REALMD_CONF="/opt/turtlewow/server/etc/realmd.conf"
REALMD_DIST="/opt/turtlewow/server/etc/realmd.conf.dist"

mkdir -p /opt/turtlewow/server/data /opt/turtlewow/server/logs /opt/turtlewow/server/honor /opt/turtlewow/server/pdump /opt/turtlewow/server/bin/patches

if [ ! -f "$MANGOS_CONF" ] && [ -f "$MANGOS_DIST" ]; then
  cp "$MANGOS_DIST" "$MANGOS_CONF"
fi

if [ ! -f "$REALMD_CONF" ] && [ -f "$REALMD_DIST" ]; then
  cp "$REALMD_DIST" "$REALMD_CONF"
fi

if [ -f "$MANGOS_CONF" ]; then
  sed -ri "s#^LoginDatabase\.Info\s*=.*#LoginDatabase.Info = \"${DB_HOST};${DB_PORT};${DB_USER};${DB_PASSWORD};tw_logon\"#" "$MANGOS_CONF"
  sed -ri "s#^WorldDatabase\.Info\s*=.*#WorldDatabase.Info = \"${DB_HOST};${DB_PORT};${DB_USER};${DB_PASSWORD};tw_world\"#" "$MANGOS_CONF"
  sed -ri "s#^CharacterDatabase\.Info\s*=.*#CharacterDatabase.Info = \"${DB_HOST};${DB_PORT};${DB_USER};${DB_PASSWORD};tw_char\"#" "$MANGOS_CONF"
  sed -ri "s#^LogsDatabase\.Info\s*=.*#LogsDatabase.Info = \"${DB_HOST};${DB_PORT};${DB_USER};${DB_PASSWORD};tw_logs\"#" "$MANGOS_CONF"
  sed -ri "s#^Database\.AutoUpdate\.Path\s*=.*#Database.AutoUpdate.Path = \"/opt/turtlewow/sql/\"#" "$MANGOS_CONF"
  sed -ri "s#^DataDir\s*=.*#DataDir = \"/opt/turtlewow/server/data\"#" "$MANGOS_CONF"
  sed -ri "s#^LogsDir\s*=.*#LogsDir = \"/opt/turtlewow/server/logs\"#" "$MANGOS_CONF"
  sed -ri "s#^HonorDir\s*=.*#HonorDir = \"/opt/turtlewow/server/honor\"#" "$MANGOS_CONF"
  sed -ri "s#^PDumpDir\s*=.*#PDumpDir = \"/opt/turtlewow/server/pdump\"#" "$MANGOS_CONF"
fi

if [ -f "$REALMD_CONF" ]; then
  sed -ri "s#^LoginDatabaseInfo\s*=.*#LoginDatabaseInfo = \"${DB_HOST};${DB_PORT};${DB_USER};${DB_PASSWORD};tw_logon\"#" "$REALMD_CONF"
fi

if [ "$DB_WAIT_TIMEOUT" -gt 0 ]; then
  end_time=$((SECONDS + DB_WAIT_TIMEOUT))
  until mariadb-admin ping -h"$DB_HOST" -P"$DB_PORT" -u"$DB_USER" -p"$DB_PASSWORD" --silent >/dev/null 2>&1; do
    if [ "$SECONDS" -ge "$end_time" ]; then
      echo "Database wait timeout reached (${DB_WAIT_TIMEOUT}s)." >&2
      exit 1
    fi
    sleep 2
  done
fi

exec "$@"
