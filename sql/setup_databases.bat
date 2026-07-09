@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "CREATE_SQL=%SCRIPT_DIR%create_databases.sql"
set "UPDATES_DIR=%SCRIPT_DIR%database_updates"

set "MYSQL_BIN="

where mariadb >nul 2>nul
if %ERRORLEVEL% equ 0 (
    set "MYSQL_BIN=mariadb"
) else (
    where mysql >nul 2>nul
    if %ERRORLEVEL% equ 0 (
        set "MYSQL_BIN=mysql"
    )
)

if "%MYSQL_BIN%"=="" (
    echo Error: neither mariadb nor mysql client was found in PATH.
    exit /b 1
)

echo Using database client: %MYSQL_BIN%

if not exist "%CREATE_SQL%" (
    echo Error: missing "%CREATE_SQL%"
    exit /b 1
)

echo Importing create_databases.sql...
"%MYSQL_BIN%" < "%CREATE_SQL%"
if errorlevel 1 exit /b 1

if not exist "%UPDATES_DIR%\" (
    echo Error: missing database_updates directory.
    exit /b 1
)

set "FOUND_SQL=0"

for %%F in ("%UPDATES_DIR%\*.sql") do (
    set "FOUND_SQL=1"
    echo Importing %%~nxF...
    "%MYSQL_BIN%" < "%%F"
    if errorlevel 1 exit /b 1
)

if "%FOUND_SQL%"=="0" (
    echo No SQL update files found in "%UPDATES_DIR%"
    exit /b 0
)

echo All SQL files imported successfully.
exit /b 0
