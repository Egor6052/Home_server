#include "Stm32RestartHelper.h"

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <chrono>
#include <iostream>

namespace {

constexpr int      POLL_INTERVAL_SEC        = 5;
constexpr int      UART_RESPONSE_TIMEOUT_MS = 500;
constexpr uint8_t  RESTART_TIMEOUT_MIN_SEC  = 5;

struct __attribute__((packed)) ProtocolPacket_t {
    uint8_t  id;
    uint8_t  rest_time;
    uint8_t  status;
    int16_t  temperature;
    int16_t  street_temp;
    uint8_t  street_humidity;
    uint8_t  reserved[6];
    uint16_t crc;
};
static_assert(sizeof(ProtocolPacket_t) == 16, "should match PACKET_SIZE in main_decl.h on STM32");

uint16_t CalculateCRC16(const uint8_t *buffer, uint16_t length) {
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

int open_serial(const std::string& path) {
    int fd = open(path.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) return -1;

    termios opt{};
    if (tcgetattr(fd, &opt) != 0) { close(fd); return -1; }

    cfsetispeed(&opt, B9600);
    cfsetospeed(&opt, B9600);
    opt.c_cflag |= (CLOCAL | CREAD);
    opt.c_cflag &= ~PARENB;
    opt.c_cflag &= ~CSTOPB;
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    opt.c_cflag &= ~CRTSCTS;
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    opt.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
    opt.c_oflag &= ~OPOST;

    if (tcsetattr(fd, TCSANOW, &opt) != 0) { close(fd); return -1; }
    tcflush(fd, TCIOFLUSH);
    return fd;
}

} // namespace

Stm32RestartHelper::Stm32RestartHelper(std::string device_path, uint8_t device_id)
    : devicePath_(std::move(device_path)), deviceId_(device_id) {}

Stm32RestartHelper::~Stm32RestartHelper() {
    stop(); // RAII: навіть якщо викликач забув stop(), деструктор сам приєднає потік -
            // саме це раніше було ризиком з ручним std::thread у main.cpp (std::terminate,
            // якщо потік лишався joinable на момент виходу зі scope).
    if (fd_ >= 0) close(fd_);
}

bool Stm32RestartHelper::ensurePortOpen() {
    if (fd_ >= 0) return true;
    fd_ = open_serial(devicePath_);
    return fd_ >= 0;
}

void Stm32RestartHelper::start() {
    if (running_.exchange(true)) return;
    worker_ = std::thread(&Stm32RestartHelper::pollLoop, this);
}

void Stm32RestartHelper::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
}

void Stm32RestartHelper::pollLoop() {
    while (running_.load()) {
        if (pausePolling_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        if (!ensurePortOpen()) {
            std::cerr << "[Stm32RestartHelper] Порт " << devicePath_ << " недоступний, повтор через 2с\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        sendAndReceiveOnce();
        std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL_SEC));
    }
}

bool Stm32RestartHelper::sendAndReceiveOnce() {
    ProtocolPacket_t req{};
    req.id        = deviceId_;
    req.rest_time = desiredTimeoutSec_.load();
    req.crc = CalculateCRC16(reinterpret_cast<uint8_t*>(&req), sizeof(req) - sizeof(req.crc));

    if (write(fd_, &req, sizeof(req)) != static_cast<ssize_t>(sizeof(req))) {
        std::cerr << "[Stm32RestartHelper] write() не вдався\n";
        return false;
    }

    uint8_t window[sizeof(ProtocolPacket_t)];
    size_t  fill = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(UART_RESPONSE_TIMEOUT_MS);

    while (std::chrono::steady_clock::now() < deadline) {
        uint8_t byte;
        ssize_t n = read(fd_, &byte, 1);
        if (n <= 0) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); continue; }

        if (fill < sizeof(window)) window[fill++] = byte;

        if (fill == sizeof(window)) {
            ProtocolPacket_t resp;
            memcpy(&resp, window, sizeof(resp));
            uint16_t crc_calc = CalculateCRC16(window, sizeof(window) - sizeof(resp.crc));

            if (crc_calc == resp.crc && resp.id == deviceId_) {
                std::lock_guard<std::mutex> lock(snapshotMutex_);
                snapshot_.serverTemperature = resp.temperature / 100.0f;
                snapshot_.streetTemperature = resp.street_temp / 100.0f;
                snapshot_.streetHumidity    = resp.street_humidity;
                snapshot_.sensorStatus      = resp.status;
                snapshot_.isFresh           = true;
                return true;
            }
            // CRC/id не зійшлись - зсув вікна на 1 байт, ресинхронізація
            memmove(window, window + 1, sizeof(window) - 1);
            fill = sizeof(window) - 1;
        }
    }

    std::lock_guard<std::mutex> lock(snapshotMutex_);
    snapshot_.isFresh = false;
    return false;
}

void Stm32RestartHelper::setConfigSTM(const std::string& value_new_time_sec) {
    try {
        int parsed = std::stoi(value_new_time_sec);
        if (parsed < RESTART_TIMEOUT_MIN_SEC) parsed = RESTART_TIMEOUT_MIN_SEC;
        if (parsed > 254) parsed = 254;
        desiredTimeoutSec_.store(static_cast<uint8_t>(parsed));
    } catch (const std::exception&) {
        std::cerr << "[Stm32RestartHelper] setConfigSTM: не число: '" << value_new_time_sec << "'\n";
    }
}

void Stm32RestartHelper::restartSystem() {
    // Призупиняємо фоновий polling, форсуємо
    // мінімальний таймаут ОДНИМ пакетом, і мовчимо трохи довше за цей
    // таймаут - STM32 сам спрацює, бо наступного пакета не буде.
    // БЛОКУЄ виклика на ~8 секунд - якщо викликається з обробника HTTP-
    // запиту, розглянь виклик з окремого потоку, щоб не тримати з'єднання.
    // ПОТІМ
    // Додам команду в один з 6 reserved-байтів кадру (напр. 0xAA = "restart
    // now") і оброби її явно в process_packet() на STM32-боці; тоді
    // цей метод стане одним send() без сну.
    if (!ensurePortOpen()) return;

    pausePolling_.store(true);
    uint8_t saved = desiredTimeoutSec_.exchange(RESTART_TIMEOUT_MIN_SEC);
    sendAndReceiveOnce();
    std::this_thread::sleep_for(std::chrono::seconds(RESTART_TIMEOUT_MIN_SEC + 3));
    desiredTimeoutSec_.store(saved);
    pausePolling_.store(false);
}

SensorsSnapshot Stm32RestartHelper::getAllSensorsData() {
    std::lock_guard<std::mutex> lock(snapshotMutex_);
    return snapshot_;
}