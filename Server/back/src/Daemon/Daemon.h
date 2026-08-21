#pragma once

#include <iostream>
#include <string.h>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <mutex>

#include "../lib/http/httplib.h"

using json = nlohmann::json;

class Daemon {
    private:
        std::string servicePath;
        std::string program_path;
        std::string working_directory;
    
    public:
        Daemon();
        ~Daemon();

        void addToStartup();
        void restartDaemon();
        void removeFromAutostart();

        std::string absolutePath();
        void setWorkingDirectory(std::string valueworkingDirectory);
        std::string getWorkingDirectory();
        void setProgramPath(std::string valueProgramPath);
        std::string getProgramPath();
        void setServicePath(std::string valuePath);
        std::string getServicePath();
    };
