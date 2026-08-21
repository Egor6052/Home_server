#include <iostream>
#include "Http.h"
#include "../BlackBox/BlackBox.h"
#include <string>


void Http::handleDownloadBlackBoxFile(const httplib::Request& req, httplib::Response& res) {
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
        res.set_header("Content-Disposition", "attachment; filename=\"" + filename + "\"");
        res.set_content(content, "text/plain");
    } else {
        json errorResponse;
        errorResponse["status"] = "error";
        errorResponse["message"] = errorMessage;

        res.status = 404;
        res.set_content(errorResponse.dump(), "application/json");
    }
}