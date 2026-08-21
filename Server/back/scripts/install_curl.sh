#!/bin/bash
# Останавливать скрипт при любой ошибке
set -e

echo "=== [CURL] Checking system for libcurl (dev) ==="

# Проверяем, установлен ли dev-пакет libcurl (нужны заголовки curl/curl.h
# и .so для линковки — именно их ищет find_package(CURL) в CMake)
if ! dpkg -s libcurl4-openssl-dev &> /dev/null; then
    echo "[CURL] libcurl dev headers not found. Installing..."
    sudo apt update
    # Ключ -y нужен, чтобы скрипт не ждал ввода пользователя (Y/n)
    sudo apt install -y libcurl4-openssl-dev
    echo "[CURL] Installation finished successfully."
else
    echo "[CURL] libcurl dev headers already installed. Skipping."
fi