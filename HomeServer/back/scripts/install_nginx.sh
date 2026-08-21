#!/bin/bash

# Останавливать скрипт при любой ошибке
set -e

echo "=== [Nginx] Checking system for Nginx ==="

# Проверяем, установлен ли nginx
if ! command -v nginx &> /dev/null; then
    echo "[Nginx] Nginx not found. Installing..."
    sudo apt update
    # Ключ -y нужен, чтобы скрипт не ждал ввода пользователя (Y/n)
    sudo apt install -y nginx
    
    echo "[Nginx] Starting and enabling systemd service..."
    sudo systemctl start nginx
    sudo systemctl enable nginx
    echo "[Nginx] Installation finished successfully."
else
    echo "[Nginx] Nginx is already installed. Skipping."
fi