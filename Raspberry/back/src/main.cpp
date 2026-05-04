#include <iostream>
#include <thread>
#include <chrono>
#include "Daemon/Daemon.h"
#include "Server/Server.h"
#include "Http/Http.h"
#include "Logger/logger.h"

LogBuffer global_logger; 

int main() {
    LogStream log_stream;
    std::cout.rdbuf(&log_stream);

    Daemon daemon;
    Http http;
    HomeServer home_server;
    
    daemon.addToStartup();

    std::thread logic_thread([&home_server]() { 
        home_server.run_logic(); 
    });

    std::cout << "Server system initialized." << std::endl;

    std::string path_to_front = "../../front/dist";
    std::cout << "Starting HTTP Server..." << std::endl;
    
    http.start_API(home_server, path_to_front);

    if (logic_thread.joinable()) {
        logic_thread.join();
    }

    return 0;
}