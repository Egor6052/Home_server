#!/bin/bash

set -e

echo "Checking OpenSSL development package..."

if dpkg -s libssl-dev >/dev/null 2>&1; then
echo "libssl-dev is already installed."
exit 0
fi

echo "Installing libssl-dev..."

sudo apt update
sudo apt install -y libssl-dev

echo "Verifying installation..."

if [ -f /usr/include/openssl/ec.h ]; then
echo "OpenSSL development files installed successfully."
else
echo "Error: openssl/ec.h not found after installation."
exit 1
fi

openssl version
echo "Done."
