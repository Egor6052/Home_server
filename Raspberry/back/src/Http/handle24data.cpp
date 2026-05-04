#include "Http.h"
#include <iostream>
#include <string>
#include "../Server/Server.h"

void Http::handle24data(const httplib::Request& req, httplib::Response& res, HomeServer& server) {
    nlohmann::ordered_json response;

    nlohmann::json history = server.getLast24Records();

    if (history.empty()) {
        response["status"] = "empty";
        response["message"] = "No data records found for current month";
        response["data"] = nlohmann::json::array();
    } else {
        response["status"] = "success";
        response["count"] = history.size();
        response["data"] = history;
    }

    // Також можемо додати поточні дані в реальному часі
    response["latest"] = {
        {"id", server.currentData.station_id},
        {"temp", server.currentData.temp},
        {"hum", server.currentData.humidity},
        {"time", server.currentData.timestamp}
    };

    res.status = 200;
    res.set_content(response.dump(4), "application/json");
}