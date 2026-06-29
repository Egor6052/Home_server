

#pragma once
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../lib/http/httplib.h"

// Server Pi 5 UART in GPIO
// /boot/config.txt:
// enable_uart=1
// dtoverlay=uart0

// Щоб відкрити UART без sudo, користувач має бути в групі dialout:
// sudo usermod -a -G dialout $(whoami)


// тепер треба запрограмувати stm32 з бібліотекою hall. я вже зробив ініціалізацію уарта, функцію відправки повідомлення та прийняття.

// вигляд запиту на stm32:  {"mk":"1","data":"true"}
// вигляд пакету від stm32: {"id":"stm32f1xROOM00001","lat":46.43,"lng":30.69,"temp":17.6,"hum":34}

using json = nlohmann::json;

struct StationData {
    std::string station_id = "abc";
    std::string timestamp = "";
    double lat = 0.0;
    double lon = 0.0;
    double temp = 0.0;
    double humidity = 0.0;
};

struct CameraData {
    std::string devicePath;
    std::string deviceName;
    std::string alsa_hw_id;
    int port;
    bool isActive = false;
};

class HomeServer {
    private:
    // HomeServer* serverPtr;

    const std::string FIREBASE_URL =
        "https://home-server-9e586-default-rtdb.firebaseio.com/measurements.json?auth=eOpdxQjllRN3hJ2Z9bIag33HIC2LaU97GkyBRvXG";

    std::string ANSI_RESET;
    std::string ANSI_BLACK;
    std::string ANSI_RED;
    std::string ANSI_YELLOW;
    std::string ANSI_BLUE;
    std::string ANSI_WHITE;
    std::string ANSI_GREEN;

    // UART
    int uart_fd;
    int uart2_fd = -1;

    std::string path_to_db;

    public:

    HomeServer();
    ~HomeServer();

    void run_logic();
    
    StationData currentData;
    std::vector<CameraData> foundCameras;
    CameraData camera1;

    bool saveToFile();
    // void saveToDB();
    nlohmann::json getLast24Records();

    // Camera
    void start_camera();
    void searching_new_usb_cam();
    std::string find_alsa_device_for_camera(const std::string& video_path);
    void set_new_camera(const std::string& path);


    // Firebase
    void connect_to_firebase();
    void send_data_to_firebase(std::string& station_id, std::string timestamp, std::string& latitude, std::string& longitude, std::string& temperature, std::string& humidity);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    // UART
    int initUART(const std::string& device_path);
    
    void stm32_helper();
    bool gpio_uart_init(const char* device);
    bool uart_request_update();
    void update_data_from_uart();

    // Stations
    bool is_data_valid();
    std::string getCurrentDateTime();

    // Functions
    bool ping();
    void restart_daemon();
    
    // double calculate_distance_from_last_point();
    // std::string generate_id();
    // double safe_get_double(const std::string& key);
    // nlohmann::ordered_json to_json() override;
};

