#include "Http.h"
#include <thread>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>


Http::Http() {
    port = 5050;
    ip = "127.0.0.1";
    HTML_DIST_PATH = "../../front/dist";
    configuration(configPath);
}

Http::~Http() {}

std::string Http::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string Http::stripQuotes(const std::string& s) {
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') ||
         (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

void Http::configuration(std::string value_path) {
    std::ifstream inFile(value_path);
    if (!inFile.is_open()) {
        std::string err_message = "Config file not found at: " + value_path + 
                                  ", using defaults (ip: " + ip + ", port: " + 
                                  std::to_string(port) + ")";
        std::cerr << err_message << std::endl;
        BlackBox::instance().pushToBlackBox(err_message);
        return;
    }

    std::string line;
    int line_num = 0;
    
    while (std::getline(inFile, line)) {
        line_num++;
        std::string trimmed = trim(line);
        
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            std::string warn = "Invalid config line " + std::to_string(line_num) + 
                             " (no '='): " + trimmed;
            std::cerr << warn << std::endl;
            continue;
        }

        std::string key = trim(trimmed.substr(0, eq_pos));
        std::string value = stripQuotes(trim(trimmed.substr(eq_pos + 1)));

        if (key == "config_ip") {
            if (!value.empty()) {
                ip = value;
                std::cout << "Config: ip set to " << ip << std::endl;
            }
        } else if (key == "config_port") {
            if (!value.empty()) {
                try {
                    int new_port = std::stoi(value);
                    if (new_port > 0 && new_port < 65536) {
                        port = new_port;
                        std::cout << "Config: port set to " << port << std::endl;
                    } else {
                        std::string err = "Invalid port value: " + value + 
                                        " (must be 1-65535), keeping: " + 
                                        std::to_string(port);
                        std::cerr << err << std::endl;
                        BlackBox::instance().pushToBlackBox(err);
                    }
                } catch (const std::exception& e) {
                    std::string err = "Failed to parse port value: " + value + 
                                    " (" + e.what() + "), keeping: " + 
                                    std::to_string(port);
                    std::cerr << err << std::endl;
                    BlackBox::instance().pushToBlackBox(err);
                }
            }
        }
    }
    
    inFile.close();
}

void Http::addCorsHeaders(httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// Розсилка події всім підключеним клієнтам
void Http::broadcast_event(const std::string& type, json &payload) {
    if (payload.empty()) return;

    json package;
    package["type"] = type;
    package["data"] = payload;

    std::string sse_msg = "data: " + package.dump() + "\n\n";

    std::vector<httplib::DataSink*> targets_snapshot;
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        targets_snapshot = stream_clients;
    }

    for (auto* sink : targets_snapshot) {
        if (sink && sink->is_writable()) {
            sink->write(sse_msg.c_str(), sse_msg.size());
        }
    }
}

void Http::start_API(IRestartHelper& restartHelper) {
    std::thread server_thread([this, &restartHelper]() {

        httplib::Server svr;
        // Кожен відкритий SSE-клієнт тримає окремий потік,
        // тож пул має бути з запасом під кількість одночасних вкладок
        svr.new_task_queue = [] { return new httplib::ThreadPool(20); };

        svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
            Http::addCorsHeaders(res);
            res.status = 200;
        });

        svr.set_mount_point("/", this->HTML_DIST_PATH.c_str());

        // SSE
        svr.Get("/api/events", [this](const httplib::Request&, httplib::Response& res) {
            Http::addCorsHeaders(res);
            res.set_header("Cache-Control", "no-cache");
            res.set_header("Connection", "keep-alive");

            res.set_chunked_content_provider(
                "text/event-stream",
                [this](size_t /*offset*/, httplib::DataSink& sink) {
                    // Реєструємо цього клієнта в спільному списку
                    {
                        std::lock_guard<std::mutex> lock(clients_mutex);
                        stream_clients.push_back(&sink);
                    }

                    // Тримаємо з'єднання відкритим, поки клієнт живий.
                    // Дані в sink пише broadcast_event з інших потоків.
                    while (sink.is_writable()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(300));
                    }

                    // Клієнт відключився (закрив вкладку/втратив мережу) -
                    // прибираємо його, щоб broadcast_event не писав у мертвий sink
                    {
                        std::lock_guard<std::mutex> lock(clients_mutex);
                        stream_clients.erase(
                            std::remove(stream_clients.begin(), stream_clients.end(), &sink),
                            stream_clients.end());
                    }

                    return false;
                }
            );
        });



        // Get current blackbox log directory
        svr.Get("/api/blackbox-settings", [this](const httplib::Request& req, httplib::Response& res) {
            this->handleGetBlackBoxSettings(req, res);
        });

        // Update blackbox log directory
        svr.Post("/api/blackbox-settings", [this](const httplib::Request& req, httplib::Response& res) {
            this->handleSetBlackBoxSettings(req, res);
        });

        // Get all files
        svr.Get("/api/blackbox-all", [this](const httplib::Request& req, httplib::Response& res) {
            this->handleGetAllBlackBoxFiles(req, res);
        });

        // Get content of a specific blackbox log file
        svr.Get("/api/blackbox-file", [this](const httplib::Request& req, httplib::Response& res) {
            this->handleGetBlackBoxFileContent(req, res);
        });

        svr.Post("/api/blackbox-remove-file", [this](const httplib::Request& req, httplib::Response& res) {
            this->handleRemoveBlackBoxFile(req, res);
        });

        // Download a blackbox log file
        svr.Get("/api/blackbox-download", [this](const httplib::Request& req, httplib::Response& res) {
            this->handleDownloadBlackBoxFile(req, res);
        });


        // Configurations
        svr.Post("/api/rs-config", [this](const httplib::Request& req, httplib::Response& res) {
            this->handleChangeConfig(req, res);
        });

        svr.Get("/api/rs-config", [this](const httplib::Request& req, httplib::Response& res) {
            this->handleGetConfig(req, res);
        });



        // Restart system
        svr.Post("/api/restart-system", [this, &restartHelper](const httplib::Request& req, httplib::Response& res) {
            this->handleRestartSystem(req, res, restartHelper);
        });
        // Get telemetry
        svr.Get("/api/telemetry", [this, &restartHelper](const httplib::Request& req, httplib::Response& res) {
            this->handleGetTelemetry(req, res, restartHelper);
        });


        
        // GET All stm settings.
        svr.Get("/api/get-stm-settings", [this, &restartHelper](const httplib::Request& req, httplib::Response& res) {
            this->handleGetStmSettings(req, res, restartHelper);
        });

        // SET Change stm settings.
        svr.Post("/api/change-stm-settings", [this, &restartHelper](const httplib::Request& req, httplib::Response& res) {
            this->handleChangeStmSettings(req, res, restartHelper);
        });
        


        svr.Get(".*", [this](const httplib::Request& req, httplib::Response& res) {
            if (req.path.rfind("/api/", 0) == 0) {
                res.status = 404;
                res.set_content(R"({"error":"Not Found"})", "application/json");
                return;
            }

            std::ifstream index_file(this->HTML_DIST_PATH + "/index.html");
            if (index_file.is_open()) {
                std::stringstream buffer;
                buffer << index_file.rdbuf();
                res.set_content(buffer.str(), "text/html");
            } else {
                res.status = 404;
            }
        });


        std::cout << "\033[1m\033[33mServer running at http://0.0.0.0:" << this->port << "\033[0m" << std::endl;
        svr.listen("0.0.0.0", this->port);
    });

    server_thread.detach();
}