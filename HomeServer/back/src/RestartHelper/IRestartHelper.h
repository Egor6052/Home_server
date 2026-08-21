#pragma once

#include <cstdint>
#include <string>

// Server Pi 5 UART in GPIO
// /boot/config.txt:
// enable_uart=1
// dtoverlay=uart0

// Щоб відкрити UART без sudo, користувач має бути в групі dialout:
// sudo usermod -a -G dialout $(whoami)

// тепер треба запрограмувати stm32 з бібліотекою hall. я вже зробив ініціалізацію уарта, функцію відправки повідомлення та прийняття.

// вигляд запиту на stm32:  {"mk":"1","data":"true"}
// вигляд пакету від stm32: {"id":"stm32f1xROOM00001","lat":46.43,"lng":30.69,"temp":17.6,"hum":34}


struct SensorsSnapshot {
    float   serverTemperature = 0.0f;   // внутрішній датчик, °C
    float   streetTemperature = 0.0f;   // зовнішній датчик - поки завжди 0 (заглушка на STM32)
    uint8_t streetHumidity    = 0;      // зовнішній датчик - поки завжди 0 (заглушка на STM32)
    uint8_t sensorStatus      = 0;      // те саме поле status з пакета (0x00=OK, 0x02=помилка датчика темп.)
    bool    isFresh           = false;
};

class IRestartHelper {

public:
    virtual ~IRestartHelper() = default;
    
    virtual void start() = 0;
    virtual void stop()  = 0;

    virtual void setConfigSTM(const std::string& value_new_time_sec) = 0;

    // протокол потрібно розширити одним з
    // reserved-байтів і обробити на STM32 для справді миттєвого ефекту.
    virtual void restartSystem() = 0;

    virtual SensorsSnapshot getAllSensorsData() = 0;
};