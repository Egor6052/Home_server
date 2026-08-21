#!/bin/bash
# Останавливать скрипт при любой ошибке
set -e

echo "=== [nlohmann_json] Checking system for nlohmann-json3-dev ==="

# Проверяем, установлен ли dev-пакет nlohmann-json (header-only,
# но пакет также ставит nlohmann_jsonConfig.cmake, который ищет
# find_package(nlohmann_json) в CMake)
if ! dpkg -s nlohmann-json3-dev &> /dev/null; then
    echo "[nlohmann_json] nlohmann-json3-dev not found. Installing..."
    sudo apt update
    # Ключ -y нужен, чтобы скрипт не ждал ввода пользователя (Y/n)
    sudo apt install -y nlohmann-json3-dev
    echo "[nlohmann_json] Installation finished successfully."
else
    echo "[nlohmann_json] nlohmann-json3-dev already installed. Skipping."
fi