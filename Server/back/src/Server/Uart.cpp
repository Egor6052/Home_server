
#include "Server.h"
#include <iomanip>
#include <chrono>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string>
#include <nlohmann/json.hpp>
#include <thread>


int HomeServer::initUART(const std::string& device_path) {
    int fd = open(device_path.c_str(), O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY);
    if (fd < 0) {
        return -1;
    }

    // Неблокирующий режим
    fcntl(fd, F_SETFL, FNDELAY); 

    termios options{};
    if (tcgetattr(fd, &options) != 0) {
        std::cerr << "[UART] tcgetattr failed for " << device_path << std::endl;
        close(fd);
        return -1;
    }

    // Скорость 9600
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    // Настройки 8N1 (8 бит, без четности, 1 стоп-бит)
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~CRTSCTS; // Выключаем аппаратный контроль потока

    // Режим Raw (отключаем канонический режим, эхо, сигналы)
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    options.c_oflag &= ~OPOST;

    // Применяем настройки
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        std::cerr << "[UART] tcsetattr failed for " << device_path << std::endl;
        close(fd);
        return -1;
    }

    tcflush(fd, TCIOFLUSH);
    std::cout << "[UART] Initialized on " << device_path << " (Non-blocking mode)" << std::endl;
    
    return fd; // Возвращаем готовый дескриптор
}

// uart-ttl in USB
// bool HomeServer::usb_uart_init(const char *device) {
//     uart_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
//     if (uart_fd < 0)
//     {
//         std::cerr << "[UART] Failed to open device " << device << std::endl;
//         return false;
//     }
//     fcntl(uart_fd, F_SETFL, 0);
//     termios options{};
//     tcgetattr(uart_fd, &options);
//     cfsetispeed(&options, B9600);
//     cfsetospeed(&options, B9600);
//     options.c_cflag |= (CLOCAL | CREAD);
//     options.c_cflag &= ~PARENB;
//     options.c_cflag &= ~CSTOPB;
//     options.c_cflag &= ~CSIZE;
//     options.c_cflag |= CS8;
//     tcsetattr(uart_fd, TCSANOW, &options);
//     std::cout << "[UART] Initialized on " << device << std::endl;
//     return true;
// }

// Server Pi 5 UART in GPIO
bool HomeServer::gpio_uart_init(const char *device) {
    // O_NDELAY (або O_NONBLOCK) важливий, щоб read не зависав!
    uart_fd = open(device, O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY);
    if (uart_fd < 0) {
        std::cerr << "[UART] Failed to open device " << device << std::endl;
        return false;
    }

    // Це робить читання неблокуючим. Якщо даних немає, read поверне -1, а не зависне.
    fcntl(uart_fd, F_SETFL, FNDELAY); 

    termios options{};
    if (tcgetattr(uart_fd, &options) != 0) {
        std::cerr << "[UART] tcgetattr failed" << std::endl;
        close(uart_fd);
        return false;
    }

    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~CRTSCTS;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    options.c_oflag &= ~OPOST;

    if (tcsetattr(uart_fd, TCSANOW, &options) != 0) {
        std::cerr << "[UART] tcsetattr failed" << std::endl;
        close(uart_fd);
        return false;
    }

    tcflush(uart_fd, TCIOFLUSH);

    std::cout << "[UART] Initialized on " << device << " (Non-blocking mode)" << std::endl;
    return true;
}

bool HomeServer::uart_request_update() {
    if (uart_fd < 0) {
        uart_fd = initUART("/dev/serial0");
        if (uart_fd < 0) {
            std::cerr << "[UART] Port is down. Cannot send request." << std::endl;
            return false;
        }
    }
    tcflush(uart_fd, TCIFLUSH);
    std::string request = R"({"mk":"1","data":"true"})";
    int bytes = write(uart_fd, request.c_str(), request.size());
    if (bytes <= 0) {
        std::cerr << "\033[31m[UART] Failed to send request. Connection lost. Closing fd.\033[0m" << std::endl;
        close(uart_fd);
        uart_fd = -1;
        return false;
    }
    std::cout << "[UART] Sent request: " << request << std::endl;
    return true;
}


void HomeServer::update_data_from_uart() {
   if (uart_fd < 0) {
        return; 
    }

    std::string packet = "";
    char buffer[256];
    int attempts = 0;
    const int MAX_ATTEMPTS = 40; 

    while (attempts < MAX_ATTEMPTS) {
        ssize_t bytes = read(uart_fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            packet += buffer;
            if (packet.find('}') != std::string::npos) break; 
        } else if (bytes == 0 || (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            std::cerr << "\033[31m[UART] Read error or device disconnected.\033[0m" << std::endl;
            close(uart_fd);
            uart_fd = -1;
            return;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        attempts++;
    }

    if (packet.empty()) {
        std::cout << "[UART] No raw data received!" << std::endl;
        return;
    } else {
        std::cout << "[UART] RAW PACKET: " << packet << std::endl;
    }

    size_t start = packet.find_first_of('{');
    size_t end   = packet.find_last_of('}');
    
    if (start != std::string::npos && end != std::string::npos && end > start) {
        packet = packet.substr(start, end - start + 1);
    } else {
        return;
    }

    try {
        json j = json::parse(packet);
        
        if (j.contains("id"))   currentData.station_id = j["id"].get<std::string>();
        if (j.contains("lat"))  currentData.lat        = j.value("lat", 0.0);
        if (j.contains("lng"))  currentData.lon        = j.value("lng", 0.0);
        if (j.contains("temp")) currentData.temp       = j.value("temp", 0.0);
        if (j.contains("hum"))  currentData.humidity   = j.value("hum", 0.0);
        
        currentData.timestamp = getCurrentDateTime();

        std::cout << "[UART] Success: Temp=" << currentData.temp 
                << ", Hum=" << currentData.humidity << std::endl;
    
        if (saveToFile()) {
            std::cout << "\033[32m[OK] Data synced to local storage.\033[0m" << std::endl;
        } else {
            std::cerr << "\033[31m[ERROR] Failed to write data to file!\033[0m" << std::endl;
        }
    
    } catch (...) {
        std::cerr << "[UART] JSON parse error" << std::endl;
    }
}