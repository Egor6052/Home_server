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

HomeServer::HomeServer() {}

HomeServer::~HomeServer() {}

void HomeServer::db_manager(IDataManager &datamanager) {
    db_manager_ = &datamanager;
}


void HomeServer::restart_daemon() {
    std::cout << "[HomeServer] Restarting daemon via systemctl..." << std::endl;
    int result = std::system("sudo systemctl restart home_server.service");
    
    if (result != 0) {
        std::cerr << "[HomeServer] Failed to restart daemon. Error code: " << result << std::endl;
    }
}