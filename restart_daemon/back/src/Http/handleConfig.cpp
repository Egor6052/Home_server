#include "Http.h"
#include <fstream>
#include <sstream>
#include "../BlackBox/BlackBox.h"

void Http::handleGetConfig(const httplib::Request& req, httplib::Response& res) {
    json response;
    response["status"] = "success";
    response["config_ip"] = ip;
    response["config_port"] = port;
    
    res.set_header("Content-Type", "application/json");
    res.status = 200;
    res.set_content(response.dump(), "application/json");
}

void Http::handleChangeConfig(const httplib::Request& req, httplib::Response& res) {
    
    if (req.body.empty()) {
        json response;
        response["status"] = "error";
        response["error"] = "Request body is empty";
        
        res.set_header("Content-Type", "application/json");
        res.status = 400;
        res.set_content(response.dump(), "application/json");
        return;
    }

    try {
        json body = json::parse(req.body);
        json response;
        
        std::string old_ip = ip;
        int old_port = port;
        bool changed = false;
        
        if (body.contains("config_ip") && body["config_ip"].is_string()) {
            std::string new_ip = body["config_ip"];
            if (!new_ip.empty()) {
                ip = new_ip;
                changed = true;
            } else {
                response["warning"] = "config_ip is empty, keeping old value";
            }
        }
        
        if (body.contains("config_port")) {
            try {
                int new_port = body["config_port"].is_string() ?
                              std::stoi(body["config_port"].get<std::string>()) :
                              body["config_port"].get<int>();
                
                if (new_port > 0 && new_port < 65536) {
                    port = new_port;
                    changed = true;
                } else {
                    response["error"] = "Invalid port (must be 1-65535)";
                    ip = old_ip;
                    port = old_port;
                    
                    res.set_header("Content-Type", "application/json");
                    res.status = 400;
                    res.set_content(response.dump(), "application/json");
                    return;
                }
            } catch (const std::exception& e) {
                response["error"] = "Invalid port format: " + std::string(e.what());
                ip = old_ip;
                port = old_port;
                
                res.set_header("Content-Type", "application/json");
                res.status = 400;
                res.set_content(response.dump(), "application/json");
                return;
            }
        }
        
        if (changed) {
            saveConfigToFile();
            std::string log_msg = "Config changed: ip=" + ip + ", port=" + std::to_string(port);
            std::cout << log_msg << std::endl;
            BlackBox::instance().pushToBlackBox(log_msg);
        }
        
        response["status"] = "success";
        response["config_ip"] = ip;
        response["config_port"] = port;
        response["message"] = changed ? "Configuration updated" : "No changes made";
        
        res.set_header("Content-Type", "application/json");
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json response;
        response["status"] = "error";
        response["error"] = "Invalid JSON: " + std::string(e.what());
        
        std::cerr << "Config error: " << e.what() << std::endl;
        BlackBox::instance().pushToBlackBox("Config error: " + std::string(e.what()));
        
        res.set_header("Content-Type", "application/json");
        res.status = 400;
        res.set_content(response.dump(), "application/json");
    }
}

void Http::saveConfigToFile() {
    // Читаем существующий файл и сохраняем все строки
    std::vector<std::string> file_lines;
    std::set<std::string> keys_found;
    
    std::ifstream inFile(configPath);
    if (inFile.is_open()) {
        std::string line;
        while (std::getline(inFile, line)) {
            std::string trimmed = trim(line);
            
            // Проверяем, это ли наш параметр
            if ((trimmed.find("config_ip=") == 0 || trimmed.find("config_port=") == 0) &&
                (trimmed[0] != '#' && trimmed[0] != ';')) {
                // Это наш параметр, пропускаем (обновим позже)
                if (trimmed.find("config_ip=") == 0) {
                    keys_found.insert("config_ip");
                } else {
                    keys_found.insert("config_port");
                }
            } else {
                // Сохраняем остальные строки
                file_lines.push_back(line);
            }
        }
        inFile.close();
    }
    
    std::string temp_path = configPath + ".tmp";
    std::ofstream tempFile(temp_path);
    
    if (!tempFile.is_open()) {
        std::string err = "Failed to create temp config file: " + temp_path;
        std::cerr << err << std::endl;
        BlackBox::instance().pushToBlackBox(err);
        return;
    }
    
    // Пишем все старые строки, исключая текущие параметры
    for (const auto& line : file_lines) {
        tempFile << line << std::endl;
    }
    
    tempFile << "config_ip=\"" << ip << "\"" << std::endl;
    tempFile << "config_port=\"" << port << "\"" << std::endl;
    
    if (tempFile.fail()) {
        std::string err = "Failed to write to temp config file";
        std::cerr << err << std::endl;
        BlackBox::instance().pushToBlackBox(err);
        tempFile.close();
        std::remove(temp_path.c_str());
        return;
    }
    
    tempFile.close();
    
    std::string backup_path = configPath + ".bak";
    if (std::remove(backup_path.c_str()) != 0 && errno != ENOENT) {
        std::string err = "Failed to remove old backup";
        std::cerr << err << std::endl;
        std::remove(temp_path.c_str());
        return;
    }
    
    if (std::rename(configPath.c_str(), backup_path.c_str()) != 0 && errno != ENOENT) {
        std::string err = "Failed to create config backup";
        std::cerr << err << std::endl;
        BlackBox::instance().pushToBlackBox(err);
        std::remove(temp_path.c_str());
        return;
    }
    
    if (std::rename(temp_path.c_str(), configPath.c_str()) != 0) {
        std::string err = "Failed to update config file - restoring backup";
        std::cerr << err << std::endl;
        BlackBox::instance().pushToBlackBox(err);
        
        std::rename(backup_path.c_str(), configPath.c_str());
        return;
    }
    
    std::string info = "Config saved safely: ip=" + ip + ", port=" + std::to_string(port) +
                      " (backup: " + backup_path + ")";
    // std::cout << info << std::endl;
    BlackBox::instance().pushToBlackBox(info);
}