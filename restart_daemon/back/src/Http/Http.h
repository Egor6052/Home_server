#ifndef HTTP_H
#define HTTP_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <mutex>
#include <thread>
#include <chrono>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "../lib/http/httplib.h"
#include "../BlackBox/BlackBox.h"
#include "../RestartHelper/IRestartHelper.h"

using json = nlohmann::json;

class Http {
private:
    int port;
    std::string ip;

    std::string HTML_DIST_PATH;
    std::string configPath = "../conf/settings.conf";
    void configuration(std::string value_path);

    // Єдиний список клієнтів для SSE
    std::vector<httplib::DataSink *> stream_clients;
    std::mutex clients_mutex;

    void saveConfigToFile();

public:
    Http();
    ~Http();
    std::string stripQuotes(const std::string& s);
    std::string trim(const std::string& s);

    static void addCorsHeaders(httplib::Response &res);
    void broadcast_event(const std::string& type, json &payload);
    void start_API(IRestartHelper& restartHelper);

    void handleGetBlackBoxSettings(const httplib::Request& req, httplib::Response& res);
    void handleSetBlackBoxSettings(const httplib::Request& req, httplib::Response& res);
    void handleGetAllBlackBoxFiles(const httplib::Request& req, httplib::Response& res);
    void handleGetBlackBoxFileContent(const httplib::Request& req, httplib::Response& res);
    void handleRemoveBlackBoxFile(const httplib::Request& req, httplib::Response& res);
    void handleDownloadBlackBoxFile(const httplib::Request& req, httplib::Response& res);
    void handleChangeConfig(const httplib::Request& req, httplib::Response& res);
    void handleGetConfig(const httplib::Request& req, httplib::Response& res);

    void handleRestartSystem(const httplib::Request& req, httplib::Response& res, IRestartHelper& restartHelper);
    void handleGetTelemetry(const httplib::Request& req, httplib::Response& res, IRestartHelper& restartHelper);
    
    void handleGetStmSettings(const httplib::Request& req, httplib::Response& res, IRestartHelper& restartHelper);
    void handleChangeStmSettings(const httplib::Request& req, httplib::Response& res, IRestartHelper& restartHelper);

};

#endif