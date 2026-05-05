#include "Http.h"
#include <thread>
#include "../Server/Server.h"

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sys/types.h>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
#else
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

Http::Http() {

}

Http::~Http() {}

#ifdef _WIN32
static bool is_loopback_ipv4(const IN_ADDR &a) {
    // 127.0.0.0/8
    return (ntohl(a.S_un.S_addr) & 0xFF000000u) == 0x7F000000u;
}
#endif

int Http::getDashPort() { return port; }

void Http::addCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
}

void Http::broadcast_event(const std::string &type, nlohmann::ordered_json &payload) {
    if (payload.empty())
        return;

    nlohmann::ordered_json package;
    package["type"] = type;
    package["data"] = payload;

    std::string sse_msg = "data: " + package.dump() + "\n\n";

    std::vector<httplib::DataSink *> targets_snapshot;
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        targets_snapshot = stream_clients;
    }

    for (auto *sink : targets_snapshot)
    {
        if (sink && sink->is_writable())
        {
            sink->write(sse_msg.c_str(), sse_msg.size());
        }
    }
}

void Http::start_API(HomeServer &home_server) {
    std::thread server_thread([this, &home_server]() {
        this->serverPtr = &home_server;
        httplib::Server svr;

        svr.new_task_queue = [] { return new httplib::ThreadPool(20); };

        // svr.Post("/api/config/device-address", [&](const httplib::Request& req, httplib::Response& res) {
        //     this->handleConfig(req, res, home_server);
        // });

        svr.Get("/api/24data", [&](const httplib::Request& req, httplib::Response& res) {
            this->handle24data(req, res, home_server);
        });

        svr.Post("/api/camera", [&](const auto& req, auto& res) {
            this->handleCameraControl(req, res);
        });

        // if (!static_path.empty()) {
        //     bool ok = svr.set_mount_point("/", static_path.c_str());
        //     std::cout << "[Web] Mounting front-end: " << (ok ? "SUCCESS" : "FAILED") << " at " << static_path << "\n";

        //     svr.set_error_handler([static_path](const auto& req, auto& res) {
        //         if (res.status == 404 && req.path.rfind("/api/", 0) != 0) {
        //             auto index_path = (std::filesystem::path(static_path) / "index.html");
        //             std::ifstream ifs(index_path, std::ios::binary);
        //             if (ifs) {
        //                 std::stringstream ss;
        //                 ss << ifs.rdbuf();
        //                 res.set_content(ss.str(), "text/html; charset=utf-8");
        //                 res.status = 200;
        //             }
        //         }
        //     });
        // }

        svr.Options(R"(/api/.*)", [](const httplib::Request& req, httplib::Response& res) {
            Http::addCorsHeaders(res);
            res.status = 200;
        });

        svr.Get("/ping", [](const auto& req, auto& res) {
            res.set_content("pong", "text/plain");
        });

        // Daemon log
        svr.Get("/api/log", [&](const httplib::Request& req, httplib::Response& res) {
            this->handleLog(req, res, home_server);
        });

        // Restart Daemon
        svr.Post("/api/restart", [&](const httplib::Request& req, httplib::Response& res) {
            this->handleRestartDaemon(req, res, home_server);
        });

        std::cout << "Local access:  \033[1m\033[33mhttp://" << this->getDashIP() << ":" << this->port << "\033[0m" << std::endl;

        if (!svr.listen(this->_ip.c_str(), this->port)) {
            std::cerr << "Could not start server on " << this->_ip << ":" << this->port << std::endl;
        }
    });

    server_thread.detach();
}
