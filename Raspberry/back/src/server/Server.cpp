#include "Server.h"
#include <iostream>
#include <string>
#include <random>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

HomeServer::HomeServer() {

    uart_fd = -1;

    ANSI_RESET = "\u001B[0m";
    ANSI_BLACK = "\u001B[30m";
    ANSI_RED = "\u001B[31m";
    ANSI_YELLOW = "\u001B[33m";
    ANSI_BLUE = "\u001B[34m";
    ANSI_WHITE = "\u001B[37m";
    ANSI_GREEN = "\u001B[32m";

    const std::string FIREBASE_URL =
        "https://home-server-9e586-default-rtdb.firebaseio.com/measurements.json?auth=eOpdxQjllRN3hJ2Z9bIag33HIC2LaU97GkyBRvXG";
    
    path_to_db = "../../../../db";

    system("killall -9 mjpg_streamer 2>/dev/null");

    camera1.devicePath = "";
    camera1.deviceName = "No Camera";
    camera1.port = 8080;
    camera1.isActive = false;

    searching_new_usb_cam();

    if (!foundCameras.empty()) {
        set_new_camera(foundCameras[0].devicePath);
        start_camera();
    }
}

HomeServer::~HomeServer() {
    if (uart_fd >= 0) {
        close(uart_fd);
    }
}


bool HomeServer::is_data_valid() {
    if (currentData.station_id.empty() || currentData.station_id == "abc") {
        return false;
    }
    
    if (currentData.lat == 0.0 && currentData.lon == 0.0 && currentData.temp == 0.0) {
        return false;
    }

    return true;
}


void HomeServer::restart_daemon() {
    std::cout << "[HomeServer] Restarting daemon via systemctl..." << std::endl;
    int result = std::system("sudo systemctl restart home_server.service");
    
    if (result != 0) {
        std::cerr << "[HomeServer] Failed to restart daemon. Error code: " << result << std::endl;
    }
}