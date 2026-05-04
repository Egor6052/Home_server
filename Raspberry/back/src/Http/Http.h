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

class Http {
private:
    std::string _ip = "127.0.0.1";
    int port = 1616;

    std::vector<httplib::DataSink *> stream_clients;
    std::mutex clients_mutex;
    std::filesystem::path getConfigPath();

    void handle24data(const httplib::Request& req, httplib::Response& res, HomeServer& home_server);
    void handleCameraControl(const httplib::Request& req, httplib::Response& res);
    void handleLog(const httplib::Request& req, httplib::Response& res, HomeServer& home_server);
    void handleRestartDaemon(const httplib::Request& req, httplib::Response& res, HomeServer& home_server);

    HomeServer* serverPtr = nullptr;

    public:

    // bool saveConfigToFile();
    // bool loadConfigFromFile();
    // void loadConfig();

    Http();
    ~Http();
    
    std::string getDashIP();
    int getDashPort();
    static void addCorsHeaders(httplib::Response &res);
    void start_API(HomeServer &home_server);

    // test
    void broadcast_event(const std::string& type, nlohmann::ordered_json &payload);
};

#endif