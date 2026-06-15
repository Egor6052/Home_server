#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

#include "Daemon.h"


/**
 * @brief Adds the home_server application to system startup via systemd service
 * 
 * This function manages the systemd service configuration:
 * - Service file location: /etc/systemd/system/home_server.service
 * - If service doesn't exist: creates and enables it
 * - If service already exists: restarts the background service
 * 
 * If running as a service (INVOCATION_ID is set), the function returns early
 * to prevent recursive restarts.
 */

void Daemon::addToStartup() {

    if (getenv("INVOCATION_ID") != nullptr) {
        // Ми запущені як сервіс. НІКОЛИ не викликаємо restart тут!
        return; 
    }
    
    std::string servicePath = getServicePath();

    if (servicePath.empty()) {
        std::cerr << "Error: servicePath is empty!" << std::endl;
        return;
    }

    // Чи існує вже файл сервісу?
    std::ifstream checkFile(servicePath);
    if (checkFile.good()) {
        checkFile.close();
        std::cout << "[Daemon] Service file already exists. Restarting home_server.service..." << std::endl;
        
        // Цей код спрацює тільки якщо ви запустили ./triton вручну з терміналу
        system("sudo systemctl restart home_server.service");
        
        // Важливо: після того, як ми дали команду на перезапуск фонового сервісу, 
        // цей "ручний" процес має завершитися, щоб не заважати.
        std::cout << "[Daemon] Background service restart triggered. Manual process exiting." << std::endl;
        exit(0); 
    }
    checkFile.close();

    // якщо файлу немає
    std::ofstream serviceFile(servicePath, std::ios::out | std::ios::trunc);
    if (!serviceFile) {
        std::cerr << "Could not open " << servicePath << " for writing. (Try running with sudo)" << std::endl;
        return;
    }

    serviceFile << "[Unit]\n";
    serviceFile << "Description=triton\n";
    serviceFile << "After=network.target\n\n";

    serviceFile << "[Service]\n";
    serviceFile << "ExecStart=" << getProgramPath() << "\n";
    serviceFile << "Restart=always\n";
    serviceFile << "User=root\n";
    serviceFile << "WorkingDirectory=" + getWorkingDirectory() + "/build\n\n";

    serviceFile << "[Install]\n";
    serviceFile << "WantedBy=multi-user.target\n";

    serviceFile.close();

    // Активація
    std::cout << "[Daemon] Configuring new service..." << std::endl;
    
    system("sudo systemctl daemon-reload");
    system("sudo systemctl enable home_server.service");
    int startStatus = system("sudo systemctl start home_server.service");

    if (startStatus == 0) {
        std::cout << "\033[1m\033[33mDaemon created and started successfully!\033[0m" << std::endl;
    } else {
        std::cerr << "Error: Failed to start new service!" << std::endl;
    }
}