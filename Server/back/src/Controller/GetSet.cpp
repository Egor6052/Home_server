#include "controller.h"
#include <fstream>

namespace {

    std::string clean_value(std::string s) {
        while (!s.empty() && (s.back() == '\r' || s.back() == '\n' ||
                            s.back() == ' ' || s.back() == '\t')) {
            s.pop_back();
        }
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
            s = s.substr(1, s.size() - 2);
        }
        return s;
    }
}

std::string HomeServer::safeGetFirebaseUrl(std::string path) {
    std::string url;
    std::ifstream file(path);
    if (!file.is_open()) {
        return url;
    }

    std::string line;
    const std::string key = "firebase_url=";
    while (std::getline(file, line)) {
        std::size_t pos = line.find(key);
        if (pos != std::string::npos) {
            url = line.substr(pos + key.size());
            break;
        }
    }

    return url;
}

std::string HomeServer::getUartPath(std::string config_path) {
    std::string uartPath;
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return uartPath;
    }

    std::string line;
    const std::string key = "watchlog_uart=";
    while (std::getline(file, line)) {
        std::size_t pos = line.find(key);
        if (pos != std::string::npos) {
            uartPath = clean_value(line.substr(pos + key.size()));
            break;
        }
    }
    return uartPath;
}


int HomeServer::getServerRestartTimeSec(std::string config_path) {
    int time_sec = 0;
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return time_sec;
    }

    std::string line;
    const std::string key = "server_restart_time_sec=";
    while (std::getline(file, line)) {
        std::size_t pos = line.find(key);
        if (pos != std::string::npos) {
            std::string value = line.substr(pos + key.size());
            try {
                time_sec = std::stoi(value);
            } catch (...) {
                time_sec = 0;
            }
            break;
        }
    }
    return time_sec;
}