#pragma once

#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

// Server Pi 5 UART in GPIO
// /boot/config.txt:
// enable_uart=1
// dtoverlay=uart0

// Щоб відкрити UART без sudo, користувач має бути в групі dialout:
// sudo usermod -a -G dialout $(whoami)

class IRestartHelper {

public:
    virtual ~IRestartHelper() = default;

    virtual void start() = 0;
    virtual void stop()  = 0;

    virtual bool changeConfigSecRestart(int new_value) = 0;

    virtual bool restartSystem() = 0;

    virtual nlohmann::ordered_json getTelemetryJson() const = 0;
    virtual std::string getSettingsPath() = 0;
};
