// uart2.cpp
// UART2 — STM32 на /dev/ttyS2
// STM32 шле:        {"heartbeat":N}
// Сервер відповідає: {"heartbeat":N}
#include "Server.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sstream>
#include <iostream>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>

void HomeServer::stm32_helper() {
    while (uart2_fd < 0) {
        uart2_fd = initUART("/dev/ttyS2");
        if (uart2_fd < 0) {
            std::cerr << "[UART2] Retrying in 2s...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    std::cout << "[UART2] STM32 listener started\n";
    std::string rx_buffer = "";

    while (true) {
        char buffer[256];
        ssize_t bytes = read(uart2_fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            rx_buffer += buffer;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        size_t start = rx_buffer.find('{');
        if (start == std::string::npos) {
            if (rx_buffer.length() > 512) rx_buffer.clear();
            continue;
        }
        size_t end = rx_buffer.find('}', start);
        if (end == std::string::npos) {
            continue;
        }
        std::string packet = rx_buffer.substr(start, end - start + 1);
        rx_buffer = rx_buffer.substr(end + 1);

        try {
            auto j = nlohmann::json::parse(packet);
            uint32_t hb = j.value("heartbeat", 0);

            std::ostringstream oss;
            oss << "{\"heartbeat\":" << hb << "}\r\n";
            std::string response = oss.str();
            write(uart2_fd, response.c_str(), response.size());
        } catch (...) {
            std::cerr << "[UART2] JSON parse error: " << packet << "\n";
        }
    }
}