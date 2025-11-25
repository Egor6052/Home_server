#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "../headers/firebase.h"

using json = nlohmann::json;

std::string ANSI_RESET = "\u001B[0m";
std::string ANSI_BLACK = "\u001B[30m";
std::string ANSI_RED = "\u001B[31m";
std::string ANSI_YELLOW = "\u001B[33m";
std::string ANSI_BLUE = "\u001B[34m";
std::string ANSI_WHITE = "\u001B[37m";
std::string ANSI_GREEN = "\u001B[32m";

const std::string FIREBASE_URL =
    "https://home-server-9e586-default-rtdb.firebaseio.com/measurements.json?auth=Your_Key";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void connect_to_firebase() {
    curl_global_init(CURL_GLOBAL_ALL);
    std::cout << "[Firebase] Ready to send data via REST API." << std::endl;
}

void send_data_to_firebase(
    std::string& station_id,
    std::string timestamp,
    std::string& latitude,
    std::string& longitude,
    std::string& temperature,
    std::string& humidity ) {
    
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    // === Формуємо JSON як у Firestore ===
    json j;
    try {
        j["station_id"]   = station_id;
        j["timestamp"]   = timestamp;
        j["lat"]         = std::stod(latitude);
        j["lng"]         = std::stod(longitude);
        j["temperature"] = std::stod(temperature);
        j["humidity"]    = std::stoi(humidity);
    }
    catch (const std::exception& e) {
        std::cerr << "[Error] Data conversion failed: " << e.what() << std::endl;
        return;
    }

    // Перетворюємо JSON у рядок
    std::string json_data = j.dump();

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, FIREBASE_URL.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());

        // Заголовки JSON
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Запис відповіді
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Відправка
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            std::string error = ANSI_RED + std::string("Failed: ") + curl_easy_strerror(res) + ANSI_RESET;
            std::cerr << error << std::endl;
        } else {
            std::cout << ANSI_GREEN + "Data sent! Response: " << readBuffer << ANSI_RESET << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}
