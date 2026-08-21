#pragma once
#include "IRestartHelper.h"
#include "../BlackBox/BlackBox.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>

struct StmTelemetry {
    float   serverTemperature = 0.0f;   // sensor_data.temperature на STM32, °C
    float   streetTemperature = 0.0f;   // weather_station.temp на STM32, °C
    uint8_t streetHumidity    = 0;      // weather_station.humidity на STM32, %
    uint8_t status            = 0;      // 0x00=OK, 0x01=Error V, 0x02=Error T (поки завжди 0x00 на STM32)
    uint8_t sttmRestartTimeoutSec = 0;  // яке watchdog-значення STM32 підтвердив, що зараз тримає
    bool    isFresh           = false;  // false, якщо останній обмін не підтверджено (CRC/id/тайм-аут)
};

class Helper : public IRestartHelper {
public:
    explicit Helper(std::string device_path = "/dev/ttyAMA0",
                     uint8_t device_id = 100);
    ~Helper();

    void start() override;
    void stop() override;

    bool changeConfigSecRestart(int new_value) override;
    bool restartSystem() override;
    nlohmann::ordered_json getTelemetryJson() const override;

    std::string stripQuotes(const std::string& s);
    std::string trim(const std::string& s);
    std::string getSettingsPath() override;

private:
    void pollLoop();
    bool sendAndReceiveOnce();
    bool ensurePortOpen();

    bool lastWriteSucceeded_ = false;

    std::string devicePath_;
    uint8_t     deviceId_;
    int         fd_ = -1;

    std::mutex portMutex_;

    int new_time_sec_restart = 0;
    std::string configPath = "../conf/stm_settings.conf";
    void configuration(std::string value_path);

    std::atomic<bool> running_{false};
    std::atomic<bool> pausePolling_{false};
    std::thread        worker_;

    std::atomic<bool> restartInProgress_{false};
    uint8_t lastSentTimeoutSec_ = 0xFF;

    std::atomic<uint8_t> pendingRestartCommand_{0x00};

    mutable std::mutex snapshotMutex_;
    StmTelemetry        snapshot_;
};
