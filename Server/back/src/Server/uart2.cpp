// uart2.cpp
// UART2 — STM32 на /dev/ttyS2
// STM32 шле:        {"heartbeat":N,"command":"get_data"}
// Сервер відповідає: {"heartbeat":N,"timestamp":NNNN,"temperature":XX.X,"humidity":XX.X}

#include "Server.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <ctime>
#include <sstream>
#include <iostream>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>

bool HomeServer::initUART2() {
    if (uart2_fd >= 0) return true;

    uart2_fd = open("/dev/ttyS2", O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY);
    if (uart2_fd < 0) {
        std::cerr << "[UART2] Failed to open /dev/ttyS2" << std::endl;
        return false;
    }

    // Неблокуючий режим — як у gpio_uart_init
    fcntl(uart2_fd, F_SETFL, FNDELAY);

    termios options{};
    if (tcgetattr(uart2_fd, &options) != 0) {
        std::cerr << "[UART2] tcgetattr failed" << std::endl;
        close(uart2_fd);
        uart2_fd = -1;
        return false;
    }

    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    options.c_cflag |=  (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |=  CS8;
    options.c_cflag &= ~CRTSCTS;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    options.c_oflag &= ~OPOST;

    if (tcsetattr(uart2_fd, TCSANOW, &options) != 0) {
        std::cerr << "[UART2] tcsetattr failed" << std::endl;
        close(uart2_fd);
        uart2_fd = -1;
        return false;
    }

    tcflush(uart2_fd, TCIOFLUSH);
    std::cout << "[UART2] Initialized on /dev/ttyS2 (Non-blocking mode)" << std::endl;
    return true;
}

void HomeServer::stm32_helper() {
    while (!initUART2()) {
        std::cerr << "[UART2] Retrying in 2s...\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
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
        std::cerr << "[UART2] RX: " << packet << "\n";

        try {
            auto j = nlohmann::json::parse(packet);

            if (j.value("command", "") != "get_data") continue;

            uint32_t hb      = j.value("heartbeat", 0);
            uint32_t unix_ts = static_cast<uint32_t>(std::time(nullptr));

            std::ostringstream oss;
            oss << "{\"heartbeat\":"    << hb
                << ",\"timestamp\":"    << unix_ts
                << ",\"temperature\":"  << currentData.temp
                << ",\"humidity\":"     << currentData.humidity
                << "}\r\n";

            std::string response = oss.str();
            write(uart2_fd, response.c_str(), response.size());
            std::cout << "[UART2] TX: " << response;

        } catch (...) {
            std::cerr << "[UART2] JSON parse error: " << packet << "\n";
        }
    }
}