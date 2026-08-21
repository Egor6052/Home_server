#pragma once

#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include <mutex>
#include <iomanip>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <thread>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <poll.h>

struct ExternalStationData {
    std::string station_id = "abc";
    std::string timestamp = "";
    double lat = 0.0;
    double lon = 0.0;
    double temp = 0.0;
    double humidity = 0.0; 

};

// Server data
struct ServerLiveData {
    float   server_temperature = 0.0f;
    uint8_t sensor_status      = 0;      // те саме поле status з пакета STM32 (0x00=OK, 0x02=помилка датчика температури)
    bool    last_poll_ok       = false;  // чи вдався останній цикл опитування (валідний CRC+id прийшов вчасно)
};

// Data for stm32
struct __attribute__((packed)) ProtocolPacket_t {
    uint8_t id;
    uint8_t restart_time;
    uint8_t status;
    int16_t server_temperature;
    int16_t street_temp;
    uint8_t street_humidity;
    uint8_t reserved[6];     // запас под будущие команды
    uint16_t crc;
};
static_assert(sizeof(ProtocolPacket_t) == 16, "должно совпадать с PACKET_SIZE на STM32");

class HomeServer {

private:
    const std::string secret_confPath = "../conf/config.properties";
    const std::string configPath = "../conf/settings.conf";
    std::string FIREBASE_URL;
    std::string STM32_UART_DEVICE;
    int SERVER_RESTART_TIME_SEC;

    float kTempScale = 0.01f;
    int kUartResponseTimeoutMs = 300; // на весь цикл запрос+ответ
    

    std::string safeGetFirebaseUrl(std::string path);
    std::string getUartPath(std::string config_path);
    int getServerRestartTimeSec(std::string config_path);

    int uart2_fd = -1;
    std::mutex liveDataMutex;

public:
    HomeServer();
    ~HomeServer();

    void run_server();
    ExternalStationData currentData;
    ServerLiveData liveData;

    void Watchlog();

    bool waitReadable(int fd, int timeout_ms);
    bool readExact(int fd, uint8_t* buf, size_t len, int timeout_ms);
    bool writeExact(int fd, const uint8_t* buf, size_t len, int timeout_ms);
    
    int initUART(const std::string& device_path);
    void update_server_live_data();
    uint16_t CalculateCRC16(const uint8_t *buffer, uint16_t length);

    // TODO: Firebase migrate to the DataManager module
    void connect_to_firebase();
    void send_data_to_firebase(std::string& station_id, std::string timestamp, std::string& latitude, std::string& longitude, std::string& temperature, std::string& humidity);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);


};


