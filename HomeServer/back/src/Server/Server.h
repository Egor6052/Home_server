

#pragma once
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../lib/http/httplib.h"
#include "IDataManager.h"

using json = nlohmann::json;

// struct StationData {
//     std::string station_id = "abc";
//     std::string timestamp = "";
//     double lat = 0.0;
//     double lon = 0.0;
//     double temp = 0.0;
//     double humidity = 0.0;
// };

 
// struct ServerLiveData {
//     float   server_temperature = 0.0f;
//     uint8_t sensor_status      = 0;      // те саме поле status з пакета STM32 (0x00=OK, 0x02=помилка датчика температури)
//     bool    last_poll_ok       = false;  // чи вдався останній цикл опитування (валідний CRC+id прийшов вчасно)
// };
 
class HomeServer {
    private:

    std::string ANSI_RESET;
    std::string ANSI_BLACK;
    std::string ANSI_RED;
    std::string ANSI_YELLOW;
    std::string ANSI_BLUE;
    std::string ANSI_WHITE;
    std::string ANSI_GREEN;

    IDataManager* db_manager_ = nullptr;

    public:

    HomeServer();
    ~HomeServer();

    void run_logic();
    
    // StationData currentData;
    // ServerLiveData liveData;
    uint8_t desired_restart_timeout_sec = 0xFF;

    void db_manager(IDataManager &datamanager);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    std::string getCurrentDateTime();

    // Functions
    bool ping();
    void restart_daemon();

};

