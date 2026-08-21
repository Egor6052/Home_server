#include "Stm32RestartHelper.h"

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>

namespace {

constexpr int      POLL_INTERVAL_SEC        = 5;
constexpr int      UART_RESPONSE_TIMEOUT_MS = 500;
constexpr uint8_t  RESTART_TIMEOUT_MIN_SEC  = 5;    /* дзеркалить RESTART_TIMEOUT_MIN_SEC на STM32 */
constexpr uint16_t RESTART_TIMEOUT_MAX_SEC = 65534;

constexpr uint8_t  RESTART_CMD_NONE     = 0x00;
constexpr uint8_t  RESTART_CMD_IMMEDIATE = 0x01;  /* main_decl.h: process_packet() -> trigger_restart() */

/* Байт-у-байт відповідає ProtocolPacket_t у main_decl.h на STM32 (16 байт,
   PACKET_SIZE). Порядок і типи полів МАЮТЬ лишатись ідентичними - інакше
   CRC й розбір пакета розсиплються на обох сторонах. */
struct __attribute__((packed)) ProtocolPacket_t {
    uint8_t  id;
    uint16_t rest_time; // число = змінити watchdog-таймаут, 0xFFFF = не змінювати
    int16_t  temperature;   // ЗАПИТ: 0/1-флаг "звітувати?". ВІДПОВІДЬ: sensor_data.temperature, *100
    int16_t  street_temp;   // ЗАПИТ: 0/1-флаг. ВІДПОВІДЬ: weather_station.temp, *100
    uint8_t  street_humidity;   // ЗАПИТ: 0/1-флаг. ВІДПОВІДЬ: weather_station.humidity, % (без *100)
    uint8_t  restart_command;   // 0x00 = нічого, 0x01 = негайний рестарт, 0x02 = скинути таймаут
    uint8_t  status;    // ВІДПОВІДЬ: 0x00=OK, 0x01=Error V, 0x02=Error T
    uint8_t  culler_temp;
    uint8_t  culler_status;
    uint8_t  reserved[2];
    uint16_t crc;
};
static_assert(sizeof(ProtocolPacket_t) == 16, "must match PACKET_SIZE in main_decl.h on STM32");

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

uint16_t clamp_timeout(int value) {
    if (value < RESTART_TIMEOUT_MIN_SEC) return RESTART_TIMEOUT_MIN_SEC;
    if (value > RESTART_TIMEOUT_MAX_SEC) return RESTART_TIMEOUT_MAX_SEC;
    return static_cast<uint16_t>(value);
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

Helper::Helper(std::string device_path, uint8_t device_id)
    : devicePath_(std::move(device_path)), deviceId_(device_id) {
    configuration(configPath);
}

Helper::~Helper() {
    stop(); // RAII: навіть якщо викликач забув stop(), деструктор сам приєднає потік -
            // саме це раніше було ризиком з ручним std::thread у main.cpp (std::terminate,
            // якщо потік лишався joinable на момент виходу зі scope).
    if (fd_ >= 0) close(fd_);
}

std::string Helper::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string Helper::stripQuotes(const std::string& s) {
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') ||
         (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

void Helper::configuration(std::string value_path) {
    std::ifstream inFile(value_path);
    if (!inFile.is_open()) {
        std::string err_message = "Config file not found at: " + value_path +
                                  ", using defaults (new_time_sec_restart: " + std::to_string(new_time_sec_restart) +
                                  ")";
        std::cerr << err_message << std::endl;
        BlackBox::instance().pushToBlackBox(err_message);
        return;
    }

    std::string line;
    int line_num = 0;

    while (std::getline(inFile, line)) {
        line_num++;
        std::string trimmed = trim(line);

        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            std::string warn = "Invalid config line " + std::to_string(line_num) +
                             " (no '='): " + trimmed;
            std::cerr << warn << std::endl;
            continue;
        }

        std::string key = trim(trimmed.substr(0, eq_pos));
        std::string value = stripQuotes(trim(trimmed.substr(eq_pos + 1)));

        if (key == "new_time_sec_restart") {
            if (!value.empty()) {
                try {
                    new_time_sec_restart = std::stoi(value);
                    std::cout << "Config: new_time_sec_restart set to " << new_time_sec_restart << std::endl;
                } catch (const std::exception& e) {
                    std::string err = "Failed to parse new_time_sec_restart value: " + value +
                                    " (" + e.what() + "), keeping: " + std::to_string(new_time_sec_restart);
                    std::cerr << err << std::endl;
                    BlackBox::instance().pushToBlackBox(err);
                }
            }
        }
    }

    inFile.close();
}

bool Helper::changeConfigSecRestart(int new_value) {
    std::ifstream inFile(getSettingsPath());
    if (!inFile.is_open()) {
        std::string err_message = "Config file not found at: " + getSettingsPath();
        std::cerr << err_message << std::endl;
        BlackBox::instance().pushToBlackBox(err_message);
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    bool keyFound = false;

    while (std::getline(inFile, line)) {
        std::string trimmed = trim(line);

        if (!trimmed.empty() && trimmed[0] != '#' && trimmed[0] != ';') {
            size_t eq_pos = trimmed.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = trim(trimmed.substr(0, eq_pos));
                if (key == "new_time_sec_restart") {
                    line = "new_time_sec_restart=" + std::to_string(new_value);
                    keyFound = true;
                }
            }
        }

        lines.push_back(line);
    }
    inFile.close();

    if (!keyFound) {
        // ключа не було у файлі - дописуємо його в кінець
        lines.push_back("new_time_sec_restart=" + std::to_string(new_value));
    }

    std::ofstream outFile(getSettingsPath(), std::ios::trunc);
    if (!outFile.is_open()) {
        std::string err_message = "Failed to open config file for writing: " + getSettingsPath();
        std::cerr << err_message << std::endl;
        BlackBox::instance().pushToBlackBox(err_message);
        return false;
    }

    for (const auto& l : lines) {
        outFile << l << "\n";
    }
    outFile.close();

    configuration(configPath);
    return true;
}


std::string Helper::getSettingsPath() {
    return this->configPath;
}


bool Helper::ensurePortOpen() {
    if (fd_ >= 0) return true;
    fd_ = open_serial(devicePath_);
    return fd_ >= 0;
}

void Helper::start() {
    if (running_.exchange(true)) return;
    worker_ = std::thread(&Helper::pollLoop, this);
}

void Helper::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
}

void Helper::pollLoop() {
    while (running_.load()) {
        if (pausePolling_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        if (!ensurePortOpen()) {
            std::cerr << "[Helper] Port " << devicePath_ << " unavailable, retry in 2s\n";
            std::lock_guard<std::mutex> lock(snapshotMutex_);
            snapshot_.isFresh = false;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        sendAndReceiveOnce();
        
        // std::cout << "[Helper] Telemetry: serverTemp=" << snapshot_.serverTemperature
        //           << "°C, streetTemp=" << snapshot_.streetTemperature
        //           << "°C, streetHumidity=" << static_cast<int>(snapshot_.streetHumidity)
        //           << "%, status=0x" << std::hex << static_cast<int>(snapshot_.status)
        //           << ", stmRestartTimeoutSec=" << std::dec << static_cast<int>(snapshot_.sttmRestartTimeoutSec)
        //           << ", isFresh=" << (snapshot_.isFresh ? "true" : "false") << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL_SEC));
    }
}

bool Helper::sendAndReceiveOnce() {
    ProtocolPacket_t req{};
    req.id = deviceId_;

    /* rest_time: лише якщо конфіг реально відрізняється від того, що STM32
       вже підтвердив - інакше 0xFFFF, щоб не слати одне й те саме щоцикл. */
    uint16_t desired = clamp_timeout(new_time_sec_restart);
    req.rest_time = (desired != lastSentTimeoutSec_) ? desired : 0xFFFF;

    /* Разова команда від restartSystem() (якщо була запланована) - читаємо
       й одразу скидаємо, щоб вона застосувалась рівно раз. */
    req.restart_command = pendingRestartCommand_.exchange(RESTART_CMD_NONE);

    /* Цей демон - єдина точка входу для телеметрії: завжди запитуємо все. */
    req.temperature     = 1;
    req.street_temp     = 1;
    req.street_humidity = 1;

    req.crc = CalculateCRC16(reinterpret_cast<uint8_t*>(&req), sizeof(req) - sizeof(req.crc));

    lastWriteSucceeded_ = (write(fd_, &req, sizeof(req)) == static_cast<ssize_t>(sizeof(req)));
    if (!lastWriteSucceeded_) {
        std::cerr << "[Helper] write() failed - closing port, will reopen next cycle\n";
        close(fd_);
        fd_ = -1;
        std::lock_guard<std::mutex> lock(snapshotMutex_);
        snapshot_.isFresh = false;
        return false;
    }

    /* ВАЖЛИВО: сам watchdog і команди на STM32 вже "зараховані" в момент
       успішного прийому й перевірки CRC/id на його боці - незалежно від
       того, чи ми дочекаємось і прочитаємо відповідь нижче. Читання ACK -
       це те, звідки береться телеметрія (і isFresh), а не умова роботи
       watchdog/команд самих по собі. */
    uint8_t window[sizeof(ProtocolPacket_t)];
    size_t  fill = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(UART_RESPONSE_TIMEOUT_MS);

    while (std::chrono::steady_clock::now() < deadline) {
        uint8_t byte;
        ssize_t n = read(fd_, &byte, 1);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            std::cerr << "[Helper] read() error, closing port\n";
            close(fd_);
            fd_ = -1;
            std::lock_guard<std::mutex> lock(snapshotMutex_);
            snapshot_.isFresh = false;
            return false;
        }
        if (n == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        if (fill < sizeof(window)) {
            window[fill++] = byte;
        }

        if (fill == sizeof(window)) {
            ProtocolPacket_t resp;
            memcpy(&resp, window, sizeof(resp));
            uint16_t crc_calc = CalculateCRC16(window, sizeof(window) - sizeof(resp.crc));

            if (crc_calc == resp.crc && resp.id == deviceId_) {
                if (req.rest_time != 0xFFFF) {
                    /* STM32 підтвердив прийом - фіксуємо це саме значення як
                       "останнє застосоване", далі шлемо 0xFF, поки не зміниться знову. */
                    lastSentTimeoutSec_ = req.rest_time;
                }

                std::lock_guard<std::mutex> lock(snapshotMutex_);
                snapshot_.serverTemperature      = resp.temperature / 100.0f;
                snapshot_.streetTemperature      = resp.street_temp / 100.0f;
                snapshot_.streetHumidity         = resp.street_humidity;
                snapshot_.status                 = resp.status;
                snapshot_.sttmRestartTimeoutSec  = resp.rest_time;
                snapshot_.isFresh                = true;
                return true;
            }
            /* CRC/id не зійшлись - зсув вікна на 1 байт, ресинхронізація
               (дзеркалить те, що робить STM32 при прийомі невалідного кадру). */
            memmove(window, window + 1, sizeof(window) - 1);
            fill = sizeof(window) - 1;
        }
    }

    /* Тайм-аут ACK - нормальний стан, коли STM32 навмисно ігнорує UART
       (RESTART_STATE_HOLDING після рестарту, до RESTART_HOLD_SEC=15с).
       Не чіпаємо lastSentTimeoutSec_ - на наступному циклі спробуємо
       застосувати те саме значення знову, поки не отримаємо ACK. */
    std::lock_guard<std::mutex> lock(snapshotMutex_);
    snapshot_.isFresh = false;
    return false;
}

bool Helper::restartSystem() {
    // restart_command=0x01
    if (restartInProgress_.exchange(true)) {
        BlackBox::instance().pushToBlackBox("[Helper] restartSystem(): already in progress, rejecting concurrent call");
        return false;
    }

    bool sent = false;
    if (ensurePortOpen()) {
        pausePolling_.store(true);
        pendingRestartCommand_.store(RESTART_CMD_IMMEDIATE);
        sendAndReceiveOnce(); // best-effort ACK; результат нижче не є визначальним
        sent = lastWriteSucceeded_;
        pausePolling_.store(false);
    }

    if (sent) {
        BlackBox::instance().pushToBlackBox("[Helper] restartSystem(): restart_command sent to STM32");
    } else {
        BlackBox::instance().pushToBlackBox("[Helper] restartSystem(): FAILED to send restart_command (port write error)");
    }

    restartInProgress_.store(false);
    return sent;
}

nlohmann::ordered_json Helper::getTelemetryJson() const {
    std::lock_guard<std::mutex> lock(snapshotMutex_);
    nlohmann::ordered_json json;
    json["serverTemperature"] = snapshot_.serverTemperature;
    json["streetTemperature"] = snapshot_.streetTemperature;
    json["streetHumidity"] = snapshot_.streetHumidity;
    json["status"] = snapshot_.status;
    json["sttmRestartTimeoutSec"] = snapshot_.sttmRestartTimeoutSec;
    json["isFresh"] = snapshot_.isFresh;
    return json;
}
