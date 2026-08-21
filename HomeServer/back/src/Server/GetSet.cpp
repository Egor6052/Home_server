#include "Server.h"
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

size_t HomeServer::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string HomeServer::getCurrentDateTime() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H:%M:%S");
    std::string fallbackTime = oss.str();

    if (!ping()) {
        return fallbackTime;
    }

    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://timeapi.io/api/Time/current/zone?timeZone=Europe/Kyiv");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HomeServer::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        // Встановлюємо жорсткі таймаути, щоб програма не "зависла" на поганому з'єднанні
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);
        
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            try {
                auto j = nlohmann::json::parse(readBuffer);
                // JSON формат від timeapi.io: {"dateTime":"2024-03-14T22:15:30.123..."}
                if (j.contains("dateTime")) {
                    std::string datetime = j["dateTime"].get<std::string>();
                    
                    if (datetime.length() >= 19) {
                        std::string formatted = datetime.substr(0, 19);
                        formatted[10] = '_';
                        return formatted;
                    }
                }
            } catch (...) {

            }
        } else {
            std::cerr << "[TimeAPI] Failed, using system clock." << std::endl;
        }
    }

    return fallbackTime;
}