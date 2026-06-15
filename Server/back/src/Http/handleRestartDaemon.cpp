#include "Http.h"
#include <iostream>
#include <string>
#include "../Server/Server.h"

void Http::handleRestartDaemon(const httplib::Request& req, httplib::Response& res, HomeServer& home_server) {
    Http::addCorsHeaders(res);
    
    nlohmann::json response;
    response["status"] = "ok";
    response["message"] = "Server is restarting...";
    
    res.set_content(response.dump(), "application/json");

    // std::thread([&home_server]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        home_server.restart_daemon();
    // }).detach();
}