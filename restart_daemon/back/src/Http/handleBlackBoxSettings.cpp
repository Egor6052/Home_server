#include <iostream>
#include "Http.h"
#include "../BlackBox/BlackBox.h"
#include <string>

void Http::handleGetBlackBoxSettings(const httplib::Request& req, httplib::Response& res) {
    Http::addCorsHeaders(res);

    json response;
    response["path"] = BlackBox::instance().getConfiguredLogPath();

    res.status = 200;
    res.set_content(response.dump(), "application/json");
}

void Http::handleSetBlackBoxSettings(const httplib::Request& req, httplib::Response& res) {
    Http::addCorsHeaders(res);

    json received_json;
    try {
        received_json = json::parse(req.body);
    } catch (...) {
        res.status = 400;
        res.set_content("{\"status\":\"error\", \"message\":\"Invalid JSON\"}", "application/json");
        return;
    }

    if (!received_json.contains("path")) {
        res.status = 400;
        res.set_content("{\"status\":\"error\", \"message\":\"Missing 'path' field\"}", "application/json");
        return;
    }

    std::string newPath = received_json["path"];
    std::string errorMessage;

    if (BlackBox::instance().setConfiguredLogPath(newPath, errorMessage)) {
        res.status = 200;
        res.set_content("{\"status\":\"success\"}", "application/json");
    } else {
        json errorResponse;
        errorResponse["status"] = "error";
        errorResponse["message"] = errorMessage;

        res.status = 400;
        res.set_content(errorResponse.dump(), "application/json");
    }
}