#include "controller.h"

uint16_t HomeServer::CalculateCRC16(const uint8_t *buffer, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)buffer[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) { crc >>= 1; crc ^= 0xA001; }
            else { crc >>= 1; }
        }
    }
    return crc;
}



int HomeServer::initUART(const std::string& device_path) {
    int fd = open(device_path.c_str(), O_RDWR | O_NOCTTY | O_SYNC | O_NDELAY);
    if (fd < 0) {
        return -1;
    }

    // Защита от гонки с демонизацией: 0/1/2 — это stdin/stdout/stderr, и
    // если на момент этого open() кто-то их временно закрыл (типичный шаг
    // демонизации), open() мог отдать нам как раз один из этих номеров.
    // Если следом демонизация переоткроет тот же номер поверх (например,
    // /dev/null для stdin) — наш UART молча подменится, и все дальнейшие
    // read/write будут падать с EBADF, хотя сам fd выглядел валидным.
    // Поэтому сразу уводим дескриптор на безопасный номер (>= 3).
    if (fd >= 0 && fd <= 2) {
        int safe_fd = fcntl(fd, F_DUPFD, 3);
        if (safe_fd < 0) {
            std::cerr << "[UART] не удалось перенести fd с " << fd << " на безопасный номер\n";
            close(fd);
            return -1;
        }
        close(fd);
        fd = safe_fd;
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
    std::cout << "[UART] Initialized on " << device_path << " (fd=" << fd << ", non-blocking)" << std::endl;
    
    return fd; // Возвращаем готовый дескриптор
}


bool HomeServer::waitReadable(int fd, int timeout_ms) {
    pollfd pfd{fd, POLLIN, 0};
    int rc = poll(&pfd, 1, timeout_ms);
    return rc > 0 && (pfd.revents & POLLIN);
}

bool HomeServer::readExact(int fd, uint8_t* buf, size_t len, int timeout_ms) {
    size_t received = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
 
    while (received < len) {
        auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining_ms <= 0) return false;
        if (!waitReadable(fd, static_cast<int>(remaining_ms))) return false;
 
        ssize_t n = read(fd, buf + received, len - received);
        if (n > 0) { received += static_cast<size_t>(n); continue; }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) continue;
        return false;
    }
    return true;
}
 
bool HomeServer::writeExact(int fd, const uint8_t* buf, size_t len, int timeout_ms) {
    size_t sent = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
 
    while (sent < len) {
        auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining_ms <= 0) {
            std::cerr << "[UART2] writeExact: таймаут, отправлено " << sent << "/" << len << " байт\n";
            return false;
        }
 
        ssize_t n = write(fd, buf + sent, len - sent);
        if (n > 0) { sent += static_cast<size_t>(n); continue; }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            pollfd pfd{fd, POLLOUT, 0};
            poll(&pfd, 1, static_cast<int>(remaining_ms));
            continue;
        }
        std::cerr << "[UART2] writeExact: fd=" << fd << ", write() вернул " << n
                   << ", errno=" << errno << " (" << std::strerror(errno) << ")\n";
        return false;
    }
    return true;
}