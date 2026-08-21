#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <filesystem>
#include "Daemon.h"
#include "../lib/http/httplib.h"
#include <string>

Daemon::Daemon() {
    absolutePath();
    setServicePath("/etc/systemd/system/home_server.service");
    setProgramPath(absolutePath() + "/build/home_server");
    setWorkingDirectory(absolutePath());
}

Daemon::~Daemon() {
}

void Daemon::restartDaemon() {
    std::cout << "Restarting daemon via systemctl..." << std::endl;
    int result = std::system("sudo systemctl restart home_server.service");
    
    if (result != 0) {
        std::cerr << "Failed to restart daemon. Error code: " << result << std::endl;
    }
}