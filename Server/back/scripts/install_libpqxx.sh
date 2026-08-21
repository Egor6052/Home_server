#!/bin/bash
# Останавливать скрипт при любой ошибке
set -e

echo "=== [libpqxx] Checking system for libpqxx-dev ==="

# Проверяем, установлен ли dev-пакет libpqxx (C++ клиент для PostgreSQL).
# В Debian/Ubuntu/Raspberry Pi OS пакет называется libpqxx-dev
# (ставит заголовки pqxx/pqxx, саму библиотеку .so и libpqxx.pc для pkg-config)
if ! dpkg -s libpqxx-dev &> /dev/null; then
    echo "[libpqxx] libpqxx-dev not found. Installing..."
    sudo apt update
    # Ключ -y нужен, чтобы скрипт не ждал ввода пользователя (Y/n)
    sudo apt install -y libpqxx-dev
    echo "[libpqxx] Installation finished successfully."
else
    echo "[libpqxx] libpqxx-dev already installed. Skipping."
fi