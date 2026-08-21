#!/bin/bash

# Останавливать скрипт при любой ошибке
set -e

echo "=== [PostgreSQL] Checking system for PostgreSQL ==="

# Проверяем, установлен ли psql
if ! command -v psql &> /dev/null; then
    echo "[PostgreSQL] Postgres not found. Installing..."
    sudo apt update
    sudo apt install -y postgresql postgresql-contrib
    
    echo "[PostgreSQL] Starting and enabling systemd service..."
    sudo systemctl start postgresql
    sudo systemctl enable postgresql
    echo "[PostgreSQL] Installation finished successfully."
else
    echo "[PostgreSQL] PostgreSQL is already installed. Skipping."
fi