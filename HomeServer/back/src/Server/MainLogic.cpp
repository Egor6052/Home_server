#include "Server.h"
#include <string>
#include <iostream>
#include <thread>

void HomeServer::run_logic() {

    while (true) {
        if (ping()) {
            connect_to_firebase();
            
            std::cout << "\n === Loop Start: " << getCurrentDateTime() << " ===" << std::endl;
            bool cycle_success = false;
            
            // Відправка запиту
            if (!uart_request_update()) {
                std::cerr << "[ERROR] Failed to send request! Retrying in 5 seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
                
            } else {
                // Чекаємо відповідь
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                update_data_from_uart();

                // Выводим только если данные реально пришли и не пустые
                if (!currentData.station_id.empty()) {
                    std::cout << "[UART] Received data -> "
                            << "id="   << currentData.station_id << ", "
                            << "lat="  << currentData.lat << ", "
                            << "lng="  << currentData.lon << ", "
                            << "temp=" << currentData.temp << "°C, "
                            << "hum="  << currentData.humidity << "%" << std::endl;
                }

                // Перевіряємо, чи дані коректні
                if (is_data_valid()) {
                    std::cout << "Data is correct. Sending to Firebase..." << std::endl;
                    
                    std::string currentTime = getCurrentDateTime();
                    std::string s_lat  = std::to_string(currentData.lat);
                    std::string s_lon  = std::to_string(currentData.lon);
                    std::string s_temp = std::to_string(currentData.temp);
                    std::string s_hum  = std::to_string((int)currentData.humidity);

                    send_data_to_firebase(
                        currentData.station_id,
                        currentTime,
                        s_lat,  
                        s_lon, 
                        s_temp,
                        s_hum   
                    );

                    cycle_success = true;
                } else {
                    std::cerr << "Data invalid (zeros or empty). Skipping upload." << std::endl;
                }
            }

            // Розумний сон
            if (cycle_success) {
                std::cout << "Success! Sleeping 1 hour..." << std::endl;
                std::cout.flush();
                std::cerr.flush();
                std::this_thread::sleep_for(std::chrono::hours(1));
            } else {
                std::cout << "Cycle failed. Retrying in 1 minute..." << std::endl;
                std::this_thread::sleep_for(std::chrono::minutes(1));
            }
        } else {
            std::cout << "[ERROR] Connection lost!" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}