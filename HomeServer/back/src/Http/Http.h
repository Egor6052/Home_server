#ifndef HTTP_H
#define HTTP_H

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "../lib/http/httplib.h"
#include "../Server/Server.h"
#include "VideoStream.h"

class Http {
private:
    std::string _ip = "127.0.0.1";
    int port = 1616;

    std::vector<httplib::DataSink *> stream_clients;
    std::mutex clients_mutex;
    std::filesystem::path getConfigPath();

    // void handle24data(const httplib::Request& req, httplib::Response& res, HomeServer& home_server);
    // void handleCameraControl(const httplib::Request& req, httplib::Response& res);
    // void handleLog(const httplib::Request& req, httplib::Response& res, HomeServer& home_server);
    void handleRestartDaemon(const httplib::Request& req, httplib::Response& res, HomeServer& home_server);
    // SSE-канал: синхронізує стан (список камер, вибрана камера, старт/стоп)
    // між усіма відкритими сторінками дашборду.
    void handleEvents(const httplib::Request& req, httplib::Response& res);

    HomeServer* serverPtr = nullptr;
    VideoStream videoStream{9002}; // WS-сервер відео (H.264/MPEG-TS), окремий порт

    public:

    Http();
    ~Http();
    
    std::string getDashIP();
    int getDashPort();
    static void addCorsHeaders(httplib::Response &res);
    void start_API(HomeServer &home_server);

    void broadcast_event(const std::string& type, nlohmann::ordered_json &payload);
};

#endif