#include "Server.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

void HomeServer::searching_new_usb_cam() {
    std::cout << "[USB] Searching for new cameras..." << std::endl;
    
    // 1. Очищуємо список знайдених камер перед новим пошуком
    foundCameras.clear(); 

    // Шлях до стабільних посилань на пристрої
    std::string path = "/dev/v4l/by-id/";

    try {
        if (fs::exists(path) && fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                std::string fullPath = entry.path().string();
                
                // Фільтр: беремо лише відеопотік (index0), ігноруємо метадані (index1 тощо)
                if (fullPath.find("video-index0") != std::string::npos) {
                    
                    CameraData newCam;
                    newCam.devicePath = fullPath;
                    // Робимо красиве ім'я з назви файлу (напр. usb-Vimicro_Corp... -> Vimicro_Corp)
                    newCam.deviceName = entry.path().filename().string();
                    newCam.isActive = false;
                    newCam.port = 8080 + foundCameras.size(); // Автоматично призначаємо наступний порт

                    foundCameras.push_back(newCam);
                    
                    std::cout << "[USB] Found: " << newCam.deviceName 
                            << " path: " << newCam.devicePath << std::endl;
                }
            }
            
            if (foundCameras.empty()) {
                std::cout << "[USB] No cameras found." << std::endl;
            }
        } else {
            std::cerr << "[USB] Error: Directory /dev/v4l/by-id/ does not exist. Connect the camera." << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[USB] File system error: " << e.what() << std::endl;
    }
}

void HomeServer::set_new_camera(const std::string& path) {
    // 2. Шукаємо камеру у векторі знайдених пристроїв за переданим шляхом
    auto it = std::find_if(foundCameras.begin(), foundCameras.end(), 
        [&path](const CameraData& cam) {
            return cam.devicePath == path;
        });

    if (it != foundCameras.end()) {
        // Якщо знайшли — копіюємо всі дані в основний об'єкт camera1
        camera1 = *it;
        camera1.isActive = true;
        
        std::cout << "[SERVER] Camera selected: " << camera1.deviceName 
                << " in port " << camera1.port << std::endl;
    } else {
        std::cerr << "[SERVER] Error: Camera with path " << path << " not found in the list!" << std::endl;
    }
}


void HomeServer::start_camera() {
    if (!camera1.isActive || camera1.devicePath.empty()) {
        std::cerr << "[CAMERA] Error: No camera selected to start!" << std::endl;
        return;
    }

    system("killall -9 mjpg_streamer ffmpeg 2>/dev/null");
    
    std::cout << "[CAMERA] Waiting for resources to be freed..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); 

    // Запуск ВІДЕО
    std::cout << "[CAMERA] Starting video streamer on " << camera1.devicePath << "..." << std::endl;
    
    // Використовуємо -n, щоб ігнорувати помилки ініціалізації динамічних параметрів
    std::string cmd = "/usr/local/bin/mjpg_streamer -i \"input_uvc.so -d " + camera1.devicePath + 
                    " -r 1280x720 -f 30 -y -n\" -o \"output_http.so -w /usr/local/share/mjpg-streamer/www -p " + 
                    std::to_string(camera1.port) + "\" &";

    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(1500)); 

    // Запуск АУДІО
    std::cout << "[CAMERA] Starting audio capture..." << std::endl;
    std::string audio_cmd = "ffmpeg -f alsa -i hw:1,0 -acodec libmp3lame -ab 128k -f mp3 -listen 1 http://0.0.0.0:8081 &";
    
    // Перевіряємо, чи запуститься відео взагалі (можна закоментувати аудіо для тесту)
    system(audio_cmd.c_str());
}