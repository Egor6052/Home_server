#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

#include "Daemon.h"

// TODO: можна зробити перевірку на наявність файлу в systemctl,
// і якщо нема тільки тоді створювати

void Daemon::addToStartup() {
    if (getServicePath().empty()) {
        std::cerr << "Error: servicePath is empty!" << std::endl;
        // // logError("Error: servicePath is empty!");
        return;
    }

    std::ofstream serviceFile(getServicePath(), std::ios::out | std::ios::trunc);
    if (!serviceFile) {
        std::cerr << "Could not open " << getServicePath() << " for writing." << std::endl;
        // // logError("Could not open " + servicePath + " for writing.");
        return;
    }

    serviceFile << "[Unit]\n";
    serviceFile << "Description=home_server\n";
    serviceFile << "After=network.target\n\n";

    serviceFile << "[Service]\n";
    serviceFile << "ExecStart=" << getProgramPath() << "\n";
    serviceFile << "Restart=always\n";
    serviceFile << "User=root\n";
    serviceFile << "WorkingDirectory=" + getWorkingDirectory() + "/build\n\n";

    serviceFile << "[Install]\n";
    serviceFile << "WantedBy=multi-user.target\n";

    serviceFile.close();

    int reloadStatus = system("sudo systemctl daemon-reload");
    if (reloadStatus != 0) {
        std::cerr << "Error: Failed to reload systemd daemon!" << std::endl;
        // // logError("Error: Failed to reload systemd daemon!");
        return;
    }

    int enableStatus = system("sudo systemctl enable home_server.service");
    if (enableStatus != 0) {
        std::cerr << "Error: Failed to enable home_server service!" << std::endl;
        // // logError("Error: Failed to enable home_server service!");
        return;
    }

    std::cout << "\033[1m\033[33mDaemon added to autostart!\033[0m" << std::endl;
}
