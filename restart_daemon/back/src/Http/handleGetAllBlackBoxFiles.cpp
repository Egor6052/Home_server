#include <iostream>
#include "Http.h"
#include "../BlackBox/BlackBox.h"
#include <string>

void Http::handleGetAllBlackBoxFiles(const httplib::Request& req, httplib::Response& res) {
    Http::addCorsHeaders(res);

    std::vector<std::string> files = BlackBox::instance().listLogFiles();

    json response;
    response["files"] = files;

    res.status = 200;
    res.set_content(response.dump(), "application/json");
}

void Http::handleGetBlackBoxFileContent(const httplib::Request& req, httplib::Response& res) {
    Http::addCorsHeaders(res);

    if (!req.has_param("name")) {
        res.status = 400;
        res.set_content("{\"status\":\"error\", \"message\":\"Missing 'name' parameter\"}", "application/json");
        return;
    }

    std::string filename = req.get_param_value("name");
    std::string content;
    std::string errorMessage;

    if (BlackBox::instance().readLogFile(filename, content, errorMessage)) {
        json response;
        response["name"] = filename;
        response["content"] = content;

        res.status = 200;
        res.set_content(response.dump(), "application/json");
    } else {
        json errorResponse;
        errorResponse["status"] = "error";
        errorResponse["message"] = errorMessage;

        res.status = 404;
        res.set_content(errorResponse.dump(), "application/json");
    }
}