#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CREATE_SQL="$SCRIPT_DIR/create_databases.sql"
UPDATES_DIR="$SCRIPT_DIR/database_updates"

MYSQL_BIN=""

if command -v mariadb >/dev/null 2>&1; then
  MYSQL_BIN="mariadb"
elif command -v mysql >/dev/null 2>&1; then
  MYSQL_BIN="mysql"
else
  echo "Error: neither mariadb nor mysql client was found in PATH."
  exit 1
fi

echo "Using database client: $MYSQL_BIN"

if [[ ! -f "$CREATE_SQL" ]]; then
  echo "Error: missing $CREATE_SQL"
  exit 1
fi

echo "Importing create_databases.sql..."
"$MYSQL_BIN" < "$CREATE_SQL"

if [[ ! -d "$UPDATES_DIR" ]]; then
  echo "Error: missing database_updates directory."
  exit 1
fi

shopt -s nullglob
SQL_FILES=("$UPDATES_DIR"/*.sql)

if [[ ${#SQL_FILES[@]} -eq 0 ]]; then
  echo "No SQL update files found in $UPDATES_DIR"
  exit 0
fi

for sql_file in "${SQL_FILES[@]}"; do
  echo "Importing $(basename "$sql_file")..."
  "$MYSQL_BIN" < "$sql_file"
done

echo "All SQL files imported successfully."
