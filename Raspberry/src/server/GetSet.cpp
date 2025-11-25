// тепер треба запрограмувати stm32 з бібліотекою hall. я вже зробив ініціалізацію уарта, функцію відправки повідомлення та прийняття.

// вигляд запиту на stm32:  {"mk":"1","data":"true"}
// вигляд пакету від stm32: {"timestamp":"2025-01-15T14:32:00Z","lat":46.43,"lng":30.69,"temp":17.6,"hum":34}

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string>
#include <nlohmann/json.hpp>

#include "../headers/main-decl.h"
#include <thread>

std::string station_id = "null";
std::string timestamp = "undefined";
std::string temperature = "0";
std::string latitude = "0";
std::string longitude = "0";
std::string humidity = "0";

using json = nlohmann::json;

// --- UART file descriptor ---
int uart_fd = -1;

// Raspberry Pi 5 UART in GPIO
// /boot/config.txt:
// enable_uart=1
// dtoverlay=uart0

// Щоб відкрити UART без sudo, користувач має бути в групі dialout:
// sudo usermod -a -G dialout $(whoami)

bool gpio_uart_init(const char *device) {
    uart_fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (uart_fd < 0)
    {
        std::cerr << "[UART] Failed to open device " << device << std::endl;
        return false;
    }

    fcntl(uart_fd, F_SETFL, 0);

    termios options{};
    if (tcgetattr(uart_fd, &options) != 0)
    {
        std::cerr << "[UART] tcgetattr failed" << std::endl;
        close(uart_fd);
        return false;
    }

    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB; // no parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;      // 8 data bits
    options.c_cflag &= ~CRTSCTS; // no flow control

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // raw mode
    options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    options.c_oflag &= ~OPOST;

    if (tcsetattr(uart_fd, TCSANOW, &options) != 0)
    {
        std::cerr << "[UART] tcsetattr failed" << std::endl;
        close(uart_fd);
        return false;
    }

    std::cout << "[UART] Initialized on " << device << std::endl;
    return true;
}


// uart-ttl in USB
bool usb_uart_init(const char *device) {
    uart_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd < 0)
    {
        std::cerr << "[UART] Failed to open device " << device << std::endl;
        return false;
    }
    fcntl(uart_fd, F_SETFL, 0);
    termios options{};
    tcgetattr(uart_fd, &options);
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    tcsetattr(uart_fd, TCSANOW, &options);
    std::cout << "[UART] Initialized on " << device << std::endl;
    return true;
}

/*
 * Запит до мікроконтролера.
 * Мікроконтролер повинен відповісти JSON пакетом.
 */
bool uart_request_update() {
    if (uart_fd < 0)
        return false;

    std::string request = R"({"mk":"1","data":"true"})";

    int bytes = write(uart_fd, request.c_str(), request.size());

    if (bytes <= 0)
    {
        std::cerr << "[UART] Failed to send request." << std::endl;
        return false;
    }

    std::cout << "[UART] Sent request: " << request << std::endl;
    return true;
}

void update_data_from_uart() {
    if (uart_fd < 0) {
        std::cout << "[UART] UART not initialized!" << std::endl;
        return;
    }

    // ВИДАЛЯЄМО tcflush(uart_fd, TCIFLUSH); 
    // Не очищайте буфер перед читанням, бо STM32 міг вже почати слати дані, поки RPi спала!

    std::string packet = "";
    char buffer[256];
    int attempts = 0;
    const int MAX_ATTEMPTS = 20; // Спроби дочитування

    // Цикл читання: накопичуємо дані, поки не знайдемо '}' або не вийде час
    while (attempts < MAX_ATTEMPTS) {
        ssize_t bytes = read(uart_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            packet += buffer;
            
            // Якщо знайшли кінець JSON, виходимо
            if (packet.find('}') != std::string::npos) {
                break; 
            }
        } else {
            // Якщо даних ще немає, трохи чекаємо (50мс)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        attempts++;
    }

    if (packet.empty()) {
        std::cout << "[UART] No data received (timeout)" << std::endl;
        return;
    }

    // Обрізка сміття (знаходимо { ... })
    size_t start = packet.find_first_of('{');
    size_t end   = packet.find_last_of('}');
    
    if (start != std::string::npos && end != std::string::npos && end > start) {
        packet = packet.substr(start, end - start + 1);
    } else {
        std::cerr << "[UART] Incomplete JSON packet: " << packet << std::endl;
        return;
    }

    std::cout << "[UART] Full packet: " << packet << std::endl;

    try {
        json j = json::parse(packet);

        // Оновлюємо глобальні змінні
        if (j.contains("id")) {
            station_id = j["id"].get<std::string>();
        }
        if (j.contains("lat")) latitude = std::to_string(j.value("lat", 0.0));
        if (j.contains("lng")) longitude = std::to_string(j.value("lng", 0.0));
        if (j.contains("temp")) temperature = std::to_string(j.value("temp", 0.0));
        if (j.contains("hum")) humidity = std::to_string(j.value("hum", 0.0));

        // ID можна просто вивести в лог, якщо треба
        if (j.contains("id")) {
            std::string dev_id = j["id"];
            std::cout << "[UART] Device ID: " << dev_id << std::endl;
        }

        // Час беремо локальний з RPi
        timestamp = getCurrentDateTime();

        std::cout << "[UART] Parsed successfully -> "
                  << "lat=" << latitude << ", lng=" << longitude
                  << ", temp=" << temperature << "°C, hum=" << humidity << "%" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[UART] JSON parse error: " << e.what() << std::endl;
        // Скидаємо на нулі при помилці
        latitude = longitude = temperature = humidity = "0";
    }
}

std::string getStationID() { return station_id; }
std::string getTimestamp() { return timestamp; }
std::string getLatitude() { return latitude; }
std::string getLongitude() { return longitude; }
std::string getTemperature() { return temperature; }
std::string getHumidity() { return humidity; }



std::string getCurrentDateTime() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}