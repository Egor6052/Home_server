#include "Http.h"
#include <iostream>
#include <string>
#include "../Server/Server.h"

void Http::handleCameraControl(const httplib::Request& req, httplib::Response& res) {
    Http::addCorsHeaders(res);
    
    std::string action = req.get_param_value("action");

    // 1. Пошук камер
    if (action == "search") {
        serverPtr->searching_new_usb_cam();
        
        // Формуємо JSON відповідь зі списком камер для фронта
        std::string json = "[";
        for (const auto& cam : serverPtr->foundCameras) {
            json += "{\"name\":\"" + cam.deviceName + "\", \"path\":\"" + cam.devicePath + "\"},";
        }
        if (json.back() == ',') json.pop_back(); // видаляємо останню кому
        json += "]";
        
        res.set_content(json, "application/json");
        return;
    }

    // 2. Вибір конкретної камери (фронт має прислати шлях)
    if (action == "select") {
        std::string selectedPath = req.get_param_value("path");
        serverPtr->set_new_camera(selectedPath);
        res.set_content("{\"status\":\"selected\"}", "application/json");
        return;
    }

    // 3. Запуск mjpg_streamer для вибраної камери
    if (action == "start") {
        if (!serverPtr->camera1.isActive) {
            res.status = 400;
            res.set_content("{\"error\":\"No camera selected\"}", "application/json");
            return;
        }

        // Вбиваємо попередній процес перед стартом нового
        system("killall -9 mjpg_streamer 2>/dev/null");

        std::string cmd = "/usr/local/bin/mjpg_streamer -i \"input_uvc.so -d " + serverPtr->camera1.devicePath + 
                        " -r 1280x720 -f 30\" -o \"output_http.so -w /usr/local/share/mjpg-streamer/www -p " + 
                        std::to_string(serverPtr->camera1.port) + "\" &";
        
        system(cmd.c_str());
        
        res.set_content("{\"status\":\"started\", \"port\":" + std::to_string(serverPtr->camera1.port) + "}", "application/json");
        return;
    }

    if (action == "stop") {
        system("killall -9 mjpg_streamer 2>/dev/null");
        system("killall -9 ffmpeg > /dev/null");

        
        serverPtr->camera1.isActive = false; // Позначаємо, що камера не активна
        res.set_content("{\"status\":\"stopped\"}", "application/json");
        return;
    }
}