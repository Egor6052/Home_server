#include "Http.h"
#include <iostream>
#include <string>

void Http::handleRestartDaemon(const httplib::Request& req, httplib::Response& res) {
    Http::addCorsHeaders(res);
    
    nlohmann::json response;
    response["status"] = "ok";
    response["message"] = "Server is restarting...";
    
    res.set_content(response.dump(), "application/json");

    // std::thread([&daemon]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // daemon.restart_daemon();
    // }).detach();
}