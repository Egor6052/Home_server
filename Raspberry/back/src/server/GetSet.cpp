#include "Server.h"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <curl/curl.h>
#include <nlohmann/json.hpp>


std::string Server::getCurrentDateTime() {
    if (!ping()) {
        using namespace std::chrono;
        auto now = system_clock::now();
        std::time_t t = system_clock::to_time_t(now);
        std::tm tm{};

        localtime_r(&t, &tm); // Для Linux/macOS. Для Windows: localtime_s(&tm, &t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d_%H:%M:%S");
        return oss.str();
    }

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://worldtimeapi.org/api/timezone/Europe/Kyiv");
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Server::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            curl_easy_cleanup(curl);
            try {
                auto j = nlohmann::json::parse(readBuffer);
                std::string datetime = j["datetime"]; 
                std::string formatted = datetime.substr(0, 19);
                formatted[10] = '_'; 
                return formatted;
            } catch (...) {
                return "Parse Error"; 
            }
        } else {
            // Виведемо конкретну помилку CURL в консоль для діагностики
            std::cerr << "[CURL Error] " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
        }
    }
    return "Network Error";
}