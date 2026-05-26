#!/usr/bin/env bash
set -euo pipefail

REALM_PUBLIC_ADDRESS="${REALM_PUBLIC_ADDRESS:-127.0.0.1}"
MANGOS_DB_USER="${MANGOS_DB_USER:-mangos}"
MANGOS_DB_PASSWORD="${MANGOS_DB_PASSWORD:-mangos}"

mysql_cmd=(mariadb -uroot -p"${MYSQL_ROOT_PASSWORD}")

"${mysql_cmd[@]}" <<SQL
CREATE USER IF NOT EXISTS '${MANGOS_DB_USER}'@'%' IDENTIFIED BY '${MANGOS_DB_PASSWORD}';
GRANT ALL PRIVILEGES ON tw_logon.* TO '${MANGOS_DB_USER}'@'%';
GRANT ALL PRIVILEGES ON tw_char.* TO '${MANGOS_DB_USER}'@'%';
GRANT ALL PRIVILEGES ON tw_world.* TO '${MANGOS_DB_USER}'@'%';
GRANT ALL PRIVILEGES ON tw_logs.* TO '${MANGOS_DB_USER}'@'%';
FLUSH PRIVILEGES;
SQL

echo "Importing create_databases.sql ..."
"${mysql_cmd[@]}" < /sql/create_databases.sql

echo "Importing sql/base/*.sql into tw_world ..."
for f in /sql/base/*.sql; do
  [ -e "$f" ] || continue
  echo "  -> $(basename "$f")"
  "${mysql_cmd[@]}" tw_world < "$f"
done

echo "Updating realmlist address to ${REALM_PUBLIC_ADDRESS} ..."
"${mysql_cmd[@]}" tw_logon <<SQL
UPDATE realmlist SET address='${REALM_PUBLIC_ADDRESS}' WHERE id=1;
SQL

echo "Database initialization completed."
