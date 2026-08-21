#include "controller.h"
#include <cstring>

HomeServer::HomeServer() {
    this->FIREBASE_URL = safeGetFirebaseUrl(secret_confPath);
    this->STM32_UART_DEVICE = getUartPath(configPath);
    this->SERVER_RESTART_TIME_SEC = getServerRestartTimeSec(configPath);

}

HomeServer::~HomeServer() {

}


/**
 * @brief Server functions:
 * Run in thread;
 * Connections for watchlog;
 * Get sensors data by UART from watchlog;
 */
void HomeServer::run_server() {
    
}


void HomeServer::update_server_live_data() {
    if (uart2_fd < 0) {
        std::lock_guard<std::mutex> lock(liveDataMutex);
        liveData.last_poll_ok = false;
        return;
    }

    ProtocolPacket_t req{};
    req.id = 100;

    int rt = SERVER_RESTART_TIME_SEC;
    if (rt < 0)   rt = 0;
    if (rt > 254) rt = 254; // 0xFF = "не менять"
    req.restart_time = static_cast<uint8_t>(rt);

    req.status = 0;
    req.server_temperature = 0;
    req.street_temp = 0;
    req.street_humidity = 0;
    req.crc = CalculateCRC16(reinterpret_cast<const uint8_t*>(&req), sizeof(req) - sizeof(req.crc));

    tcflush(uart2_fd, TCIFLUSH); // чистим хвост от прошлого неудачного цикла

    if (!writeExact(uart2_fd, reinterpret_cast<const uint8_t*>(&req), sizeof(req), kUartResponseTimeoutMs)) {
        std::cerr << "[UART2] Не удалось отправить запрос\n";
        std::lock_guard<std::mutex> lock(liveDataMutex);
        liveData.last_poll_ok = false;
        return;
    }

    ProtocolPacket_t resp{};
    if (!readExact(uart2_fd, reinterpret_cast<uint8_t*>(&resp), sizeof(resp), kUartResponseTimeoutMs)) {
        std::cerr << "[UART2] Таймаут ожидания ответа от STM32\n";
        std::lock_guard<std::mutex> lock(liveDataMutex);
        liveData.last_poll_ok = false;
        return;
    }

    uint16_t crc_calc = CalculateCRC16(reinterpret_cast<const uint8_t*>(&resp), sizeof(resp) - sizeof(resp.crc));
    if (crc_calc != resp.crc || resp.id != req.id) {
        std::cerr << "[UART2] Ответ не прошёл проверку (CRC или id)\n";
        std::lock_guard<std::mutex> lock(liveDataMutex);
        liveData.last_poll_ok = false;
        return;
    }

    std::lock_guard<std::mutex> lock(liveDataMutex);
    liveData.server_temperature = static_cast<float>(resp.server_temperature) * kTempScale;
    liveData.sensor_status      = resp.status;
    liveData.last_poll_ok       = true;

    std::cout << "TEST RESPONSE stm32: temp: " << liveData.server_temperature << std::endl;
}

