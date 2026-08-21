#ifndef HTTP_H
#define HTTP_H

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "../lib/http/httplib.h"
#include "../Controller/controller.h"

class Http {
private:
    std::string _ip = "127.0.0.1";
    int port = 1616;

    std::vector<httplib::DataSink *> stream_clients;
    std::mutex clients_mutex;
    // std::filesystem::path getConfigPath();

    void handleRestartDaemon(const httplib::Request& req, httplib::Response& res);
    HomeServer* serverPtr = nullptr;

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