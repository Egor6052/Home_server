#include "Http.h"
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sys/types.h>
#include <string>
#include <vector>

static bool is_valid_port(int p) { return p >= 1 && p <= 65535; }


void Http::handleConfig(const httplib::Request& req, httplib::Response& res, HomeServer& home_server) {
    Http::addCorsHeaders(res);

    try {
        auto j = nlohmann::ordered_json::parse(req.body);

        if (!j.contains("port")) {
            res.status = 400;
            res.set_content("{\"status\":\"error\",\"message\":\"port required\"}", "application/json");
            return;
        }

        int newPort = j["port"].get<int>();
        std::string newBind = this->_ip;
        if (j.contains("_ip")) newBind = j["_ip"].get<std::string>();

        if (!is_valid_port(newPort)) {
            res.status = 400;
            res.set_content("{\"status\":\"error\",\"message\":\"invalid port\"}", "application/json");
            return;
        }

        this->_ip = newBind;
        this->port = newPort;

        // std::cout << "http://" << _ip << ":"  << port << std::endl;
        if (!this->saveConfigToFile()) {
            res.status = 500;
            res.set_content("{\"status\":\"error\",\"message\":\"failed to write http.conf\"}", "application/json");
            return;
        }

        nlohmann::ordered_json ans;
        ans["status"] = "ok";
        ans["restart_required"] = true;
        ans["saved"] = {{"_ip", this->_ip}, {"port", this->port}};

        res.status = 200;
        res.set_content(ans.dump(), "application/json");
    } catch (const std::exception& e) {
        nlohmann::ordered_json err;
        err["status"] = "error";
        err["message"] = "Invalid JSON";
        err["details"] = e.what();
        res.status = 400;
        res.set_content(err.dump(), "application/json");
    }
}

