#include "Server.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

void HomeServer::searching_new_usb_cam() {
    std::cout << "[USB] Searching for new cameras..." << std::endl;
    
    foundCameras.clear();

    // 
    std::string path = "/dev/v4l/by-id/";

    try {
        if (fs::exists(path) && fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                std::string fullPath = entry.path().string();
                
                // Фільтр: беремо лише відеопотік (index0), ігноруємо метадані (index1 тощо)
                if (fullPath.find("video-index0") != std::string::npos) {
                    
                    CameraData newCam;
                    newCam.devicePath = fullPath;
                    newCam.deviceName = entry.path().filename().string();
                    newCam.isActive = false;
                    newCam.port = 8080 + foundCameras.size();

                    newCam.alsa_hw_id = find_alsa_device_for_camera(fullPath);

                    foundCameras.push_back(newCam);
                    
                    std::cout << "[USB] Found Video: " << newCam.deviceName 
                        << " | Path: " << newCam.devicePath 
                        << " | Audio: " << (newCam.alsa_hw_id.empty() ? "None" : newCam.alsa_hw_id) 
                        << std::endl;
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


std::string HomeServer::find_alsa_device_for_camera(const std::string& video_path) {
    // Вытаскиваем базовый ID из строки (например: "usb-046d_0825_089F8E10")
    std::string base_id = "";
    size_t last_slash = video_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        std::string filename = video_path.substr(last_slash + 1);
        size_t video_pos = filename.find("-video");
        if (video_pos != std::string::npos) {
            base_id = filename.substr(0, video_pos); // Отрезаем хвост "-video-index0"
        }
    }

    if (base_id.empty()) return "";

    std::string snd_path = "/dev/snd/by-id/";
    if (!fs::exists(snd_path)) return "";

    // Ищем связанное аудио-устройство в папке snd
    try {
        for (const auto& entry : fs::directory_iterator(snd_path)) {
            std::string snd_filename = entry.path().filename().string();
            
            // Если файл начинается с того же ID, что и камера
            if (snd_filename.find(base_id) != std::string::npos) {
                // Это симлинк (указывает на ../controlC1 или ../pcmC1D0c)
                std::string target = fs::read_symlink(entry.path()).string();
                
                // Вытаскиваем номер звуковой карты (Card ID)
                size_t c_pos = target.find("controlC");
                if (c_pos != std::string::npos) {
                    // Берем цифру после "controlC" (например "1" из "controlC1")
                    std::string card_num = target.substr(c_pos + 8);
                    
                    // Убираем возможные мусорные символы (если путь длиннее)
                    card_num.erase(std::remove_if(card_num.begin(), card_num.end(), 
                                   [](char c) { return !std::isdigit(c); }), card_num.end());
                                   
                    return "hw:" + card_num + ",0"; 
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[USB-AUDIO] Error reading ALSA paths: " << e.what() << std::endl;
    }
    
    // Если камера не имеет микрофона
    return ""; 
}

void HomeServer::set_new_camera(const std::string& path) {
    // Шукаємо камеру у векторі знайдених пристроїв за переданим шляхом
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

    // Video starting
    std::cout << "[CAMERA] Starting video streamer on " << camera1.devicePath << "..." << std::endl;
    
    std::string cmd = "/usr/local/bin/mjpg_streamer -i \"input_uvc.so -d " + camera1.devicePath + 
                    " -r 1280x720 -f 30 -y -n\" -o \"output_http.so -w /usr/local/share/mjpg-streamer/www -p " + 
                    std::to_string(camera1.port) + "\" &";
    system(cmd.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(1500)); 

    // Audio starting
    if (!camera1.alsa_hw_id.empty()) {
        std::cout << "[CAMERA] Starting audio capture on ALSA device: " << camera1.alsa_hw_id << "..." << std::endl;
        
        // Dinamic replse alsa_hw_id
        std::string audio_cmd = "ffmpeg -f alsa -i " + camera1.alsa_hw_id + " -acodec libmp3lame -ab 128k -f mp3 -listen 1 http://0.0.0.0:8081 &";
        
        system(audio_cmd.c_str());
    } else {
        std::cout << "[CAMERA] No built-in microphone detected for this camera. Skipping audio." << std::endl;
    }
}