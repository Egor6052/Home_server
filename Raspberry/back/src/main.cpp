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
    http.start_API(home_server);

    std::thread logic_thread([&home_server]() { 
        home_server.run_logic(); 
    });

    std::cout << "Server system started." << std::endl;

    if (logic_thread.joinable()) {
        logic_thread.join();
    }

    return 0;
}