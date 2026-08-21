// #include "Daemon.h"
// #include <iostream>
// #include <fstream>
// #include <filesystem>
// #include <cstdlib>

// namespace fs = std::filesystem;

// void Daemon::addNginxConfigToNginx() {
//     std::string sourcePath = absolutePath() + "/nginx.conf";
//     std::string targetPath = "/etc/nginx/conf.d/home_server.conf";

//     try {
//         if (!fs::exists(sourcePath)) {
//             std::cerr << "[Daemon] Error: Source nginx config not found at " << sourcePath << std::endl;
//             return;
//         }

//         // Копіюємо файл
//         fs::copy_fVideo streamile(sourcePath, targetPath, fs::copy_options::overwrite_existing);
//         std::cout << "[Daemon] Nginx config copied to " << targetPath << std::endl;

//         // Перевіряємо конфіг на помилки перед рестартом
//         if (std::system("nginx -t") == 0) {
//             std::system("systemctl restart nginx");
//             std::cout << "[Daemon] Nginx restarted successfully." << std::endl;
//         } else {
//             std::cerr << "[Daemon] Nginx config test failed! Not restarting." << std::endl;
//         }

//     } catch (const fs::filesystem_error& e) {
//         std::cerr << "[Daemon] Filesystem error: " << e.what() << std::endl;
//     }
// }

// void Daemon::removeNginxConfigFromNginx() {
//     std::string targetPath = "/etc/nginx/conf.d/home_server.conf";

//     try {
//         if (fs::exists(targetPath)) {
//             fs::remove(targetPath);
//             std::cout << "[Daemon] Nginx config removed." << std::endl;
            
//             if (std::system("nginx -t") == 0) {
//                 std::system("systemctl restart nginx");
//             }
//         }
//     } catch (const fs::filesystem_error& e) {
//         std::cerr << "[Daemon] Error removing config: " << e.what() << std::endl;
//     }
// }