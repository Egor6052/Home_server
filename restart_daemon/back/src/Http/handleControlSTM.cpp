#include "Http.h"
#include <fstream>
#include <sstream>
#include "../BlackBox/BlackBox.h"

void Http::handleGetTelemetry(const httplib::Request& /*req*/, httplib::Response& res, IRestartHelper& restartHelper) {
    Http::addCorsHeaders(res);
    try {
        nlohmann::ordered_json telemetry = restartHelper.getTelemetryJson();
        res.status = 200;
        res.set_content(telemetry.dump(), "application/json");
    } catch (const std::exception& e) {
        std::string err = std::string("[Http] handleGetTelemetry: ") + e.what();
        std::cerr << err << std::endl;
        BlackBox::instance().pushToBlackBox(err);
        res.status = 500;
        res.set_content(R"({"error":"Internal error while reading telemetry"})", "application/json");
    }
}

void Http::handleRestartSystem(const httplib::Request& req, httplib::Response& res, IRestartHelper& restartHelper) {
    Http::addCorsHeaders(res);
    BlackBox::instance().pushToBlackBox("System will be immediately restarted from the outside.");
    bool sent = restartHelper.restartSystem();

    json body;
    body["success"] = sent;
    if (sent) {
        res.status = 200;
        body["message"] = "Restart command sent to STM32";
    } else {
        res.status = 503;
        body["message"] = "Failed to send restart command (serial port unavailable or busy)";
    }
    res.set_content(body.dump(), "application/json");
}
