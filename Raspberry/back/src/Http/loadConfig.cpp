#include "Http.h"
#include <thread>

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sys/types.h>
#include <filesystem>


// static std::string trim_str(std::string s) {
//     auto notSpace = [](unsigned char c)
//     { return !std::isspace(c); };
//     s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
//     s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
//     return s;
// }

std::filesystem::path Http::getConfigPath() {
    namespace fs = std::filesystem;

    fs::path projectRoot = fs::current_path().parent_path().parent_path();

    return projectRoot / "back" / "Config" / "http.conf";
}


bool Http::saveConfigToFile() {
    namespace fs = std::filesystem;
    auto confPath = getConfigPath();
    fs::create_directories(confPath.parent_path());

    std::ofstream file(confPath, std::ios::trunc);
    if (!file.is_open()) {
        std::cout << "ERROR opening file: " << confPath << "\n";
        return false;
    }

    file << "server_ip=" << server_ip << "\n";
    file << "port=" << port << "\n";

    std::cout << "Config saved at: " << confPath << "\n";
    return true;
}



// void Http::loadConfig() {

//     namespace fs = std::filesystem;

//     fs::path confPath = getConfigPath();

//     std::cout << "[Http] Loading config from: " << confPath << std::endl;

//     std::ifstream file(confPath);

//     if (!file.is_open()) {

//         std::cout << "[Http] Config not found. Creating default.\n";

//         fs::create_directories(confPath.parent_path());

//         std::ofstream create(confPath);
//         create << "bind_ip=0.0.0.0\n";
//         create << "port=1515\n";
//         create << "server_ip=0.0.0.0\n";
//         create << "server_port=1515\n";
//         create.close();

//         bind_ip = "0.0.0.0";
//         port = 1515;
//         server_ip = "0.0.0.0";
//         server_port = 1515;
//         return;
//     }

//     std::string line;

//     while (std::getline(file, line))
//     {

//         line = trim_str(line);

//         if (line.empty() || line[0] == '#')
//             continue;

//         auto pos = line.find('=');
//         if (pos == std::string::npos)
//             continue;

//         std::string key = trim_str(line.substr(0, pos));
//         std::string value = trim_str(line.substr(pos + 1));

//         if (key == "bind_ip")
//             bind_ip = value;

//         else if (key == "port")
//         {
//             try
//             {
//                 port = std::stoi(value);
//             }
//             catch (...)
//             {
//                 std::cout << "[Http] Invalid port value in config\n";
//             }
//         }
//         else if (key == "server_ip")
//         {
//             server_ip = value;
//         }
//         else if (key == "server_port")
//         {
//             try
//             {
//                 server_port = std::stoi(value);
//             }
//             catch (...)
//             {
//                 std::cout << "[Http] Invalid port value in config\n";
//             }
//         }
//     }
// }









// bool Http::loadConfigFromFile() {

//     namespace fs = std::filesystem;

//     fs::path confPath = getConfigPath();

//     std::cout << "[Http] Loading config from: " << confPath << std::endl;

//     std::ifstream in(confPath);

//     if (!in.is_open())
//     {
//         std::cout << "[Http] Config not found. Creating default.\n";
//         saveConfigToFile();
//         return false;
//     }

//     std::string line;
//     while (std::getline(in, line))
//     {

//         line = trim_str(line);

//         if (line.empty() || line[0] == '#')
//             continue;

//         auto pos = line.find('=');
//         if (pos == std::string::npos)
//             continue;

//         auto key = trim_str(line.substr(0, pos));
//         auto val = trim_str(line.substr(pos + 1));

//         if (key == "bind_ip")
//             bind_ip = val;

//         else if (key == "port")
//         {
//             try
//             {
//                 port = std::stoi(val);
//             }
//             catch (...)
//             {
//                 std::cout << "[Http] Invalid port value in config\n";
//             }
//         }
//         else if (key == "server_ip")
//         {
//             server_ip = val;
//         }
//         else if (key == "server_port")
//         {
//             try
//             {
//                 server_port = std::stoi(val);
//             }
//             catch (...)
//             {
//                 std::cout << "[Http] Invalid port value in config\n";
//             }
//         }
//     }

//     std::cout << "[Http] Loaded: " << bind_ip << ":" << port << std::endl;

//     return true;
// }
