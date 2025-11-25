#include <iostream>
#include <thread>
#include <chrono>
#include "headers/main-decl.h"
#include "headers/firebase.h"
#include "headers/Daemon.h"

// Допоміжна функція — перевіряє, чи отримали реальні дані, а не "нулі"
bool is_data_valid() {
    return (
        // timestamp != "undefined" && timestamp != "" &&
            latitude != "0" && latitude != "0.0" &&
            longitude != "0" && longitude != "0.0" &&
            temperature != "0" && temperature != "0.0" &&
            humidity != "0" && humidity != "0.0");
}

int main() {
    Daemon daemon;
    daemon.addToStartup();

    connect_to_firebase();

    if (!gpio_uart_init("/dev/serial0")) {
        std::cerr << "Cannot initialize UART!" << std::endl;
        return 1;
    }

    std::cout << "UART initialized. Entering main loop..." << std::endl;

    while (true) {
        std::cout << "\n=== New cycle (once per hour) ===" << std::endl;

        // 1. Відправляємо запит до STM32
        if (!uart_request_update()) {
            std::cerr << "[ERROR] Failed to send request to STM32!" << std::endl;
        } else {
            std::cout << "[UART] Request sent successfully." << std::endl;

            // 2. Даємо STM32 час на відповідь (зазвичай 100-500 мс достатньо)
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            // 3. Читаємо відповідь
            update_data_from_uart();

            // 4. Виводимо, що отримали (для дебагу)
            std::cout << "[UART] Received data -> "
                    << "id=" << station_id << ", "
                      << "ts=" << timestamp << ", "
                      << "lat=" << latitude << ", "
                      << "lng=" << longitude << ", "
                      << "temp=" << getTemperature() << "°C, "
                      << "hum=" << humidity << "%" << std::endl;

            // 5. Перевіряємо, чи дані коректні
            if (is_data_valid()) {
                std::cout << "Data is VALID. Sending to Firebase..." << std::endl;

                send_data_to_firebase(
                    station_id,
                    getTimestamp(),
                    latitude,  
                    longitude, 
                    temperature,
                    humidity   
                );

                // Видалення старих записів з Firebase
                // delete_old_firebase_records();
            } else {
                std::cerr << "Data is INVALID or incomplete! Skipping Firebase upload." << std::endl;
            }
        }

        std::cout << "Sleeping 1 hour until next request..." << std::endl;
        std::this_thread::sleep_for(std::chrono::hours(1));
    }

    return 0;
}