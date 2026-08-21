#pragma once
#include "IRestartHelper.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <string>

class Stm32RestartHelper : public IRestartHelper {
public:
    explicit Stm32RestartHelper(std::string device_path = "/dev/ttyAMA0",
                                 uint8_t device_id = 100);
    ~Stm32RestartHelper();

    void start() override;
    void stop() override;

    void setConfigSTM(const std::string& value_new_time_sec) override;
    void restartSystem() override;
    SensorsSnapshot getAllSensorsData() override;

private:
    void pollLoop();            // тіло фонового потоку (start() запускає саме це)
    bool sendAndReceiveOnce();  // один цикл запит/відповідь; false = timeout/помилка
    bool ensurePortOpen();

    std::string devicePath_;
    uint8_t     deviceId_;
    int         fd_ = -1;

    std::atomic<bool> running_{false};
    std::atomic<bool> pausePolling_{false}; // тимчасова пауза (використовується restartSystem())
    std::thread        worker_;

    std::mutex        snapshotMutex_;
    SensorsSnapshot   snapshot_;

    std::atomic<uint8_t> desiredTimeoutSec_{0xFF}; // 0xFF = не змінювати
};