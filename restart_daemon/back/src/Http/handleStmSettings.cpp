#include "Http.h"
#include <fstream>
#include <iostream>
#include "../BlackBox/BlackBox.h"

void Http::handleGetStmSettings(const httplib::Request& /*req*/, httplib::Response& res, IRestartHelper& restartHelper) {
    Http::addCorsHeaders(res);

    std::ifstream inFile(restartHelper.getSettingsPath());
    if (!inFile.is_open()) {
        res.status = 500;
        res.set_content(R"({"error":"Settings file not found"})", "application/json");
        return;
    }

    json settings = json::object();
    std::string line;
    while (std::getline(inFile, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') continue;

        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key   = trim(trimmed.substr(0, eq_pos));
        std::string value = stripQuotes(trim(trimmed.substr(eq_pos + 1)));
        settings[key] = value;
    }

    res.status = 200;
    res.set_content(settings.dump(), "application/json");
}

void Http::handleChangeStmSettings(const httplib::Request& req, httplib::Response& res, IRestartHelper& restartHelper) {
    Http::addCorsHeaders(res);

    json body;
    try {
        body = json::parse(req.body);
    } catch (const std::exception&) {
        res.status = 400;
        res.set_content(R"({"error":"Invalid JSON body"})", "application/json");
        return;
    }

    if (!body.contains("new_time_sec_restart") || !body["new_time_sec_restart"].is_number_integer()) {
        res.status = 400;
        res.set_content(R"({"error":"new_time_sec_restart must be an integer"})", "application/json");
        return;
    }

   int new_value = body["new_time_sec_restart"].get<int>();
    bool ok = restartHelper.changeConfigSecRestart(new_value);

    json result;
    result["success"] = ok;
    res.status = ok ? 200 : 500;
    res.set_content(result.dump(), "application/json");

    std::string logMsg = ok
        ? "New restart timeout has been set: " + std::to_string(new_value)
        : "Failed to set new restart timeout to " + std::to_string(new_value) + " (settings file write error)";
    BlackBox::instance().pushToBlackBox(logMsg);
}
