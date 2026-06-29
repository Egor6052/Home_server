#include "Http.h"
#include <iostream>
#include <string>
#include "../Logger/logger.h"

void Http::handleLog(const httplib::Request& req, httplib::Response& res, HomeServer& home_server) {
    Http::addCorsHeaders(res);
    
    nlohmann::json response;
    response["logs"] = global_logger.get_all_logs_json();
    
    res.set_content(response.dump(), "application/json");
}