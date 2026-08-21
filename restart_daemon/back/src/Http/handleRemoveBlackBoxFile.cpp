#include <iostream>
#include "Http.h"
#include <string>
#include "../BlackBox/BlackBox.h"

void Http::handleRemoveBlackBoxFile(const httplib::Request& req, httplib::Response& res) {
    Http::addCorsHeaders(res);

    json received_json;
    try {
        received_json = json::parse(req.body);
    } catch (...) {
        res.status = 400;
        res.set_content("{\"status\":\"error\", \"message\":\"Invalid JSON\"}", "application/json");
        return;
    }

    if (!received_json.contains("name")) {
        res.status = 400;
        res.set_content("{\"status\":\"error\", \"message\":\"Missing 'name' field\"}", "application/json");
        return;
    }

    std::string filename = received_json["name"];
    std::string errorMessage;

    if (BlackBox::instance().removeFile(filename, errorMessage)) {
        res.status = 200;
        res.set_content("{\"status\":\"success\"}", "application/json");
        BlackBox::instance().pushToBlackBox("Cleared history. File " + filename + " was removed.");

    } else {
        json errorResponse;
        errorResponse["status"] = "error";
        errorResponse["message"] = errorMessage;

        res.status = 404;
        res.set_content(errorResponse.dump(), "application/json");
    }
}


