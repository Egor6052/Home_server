#include <iostream>
#include <thread>
#include <chrono>
#include "Daemon/Daemon.h"
#include "Controller/controller.h"
#include "Http/Http.h"

int main() {
    Daemon daemon;
    Http http;
    HomeServer home_server;
    // DataManager datamanager;
    // datamanager.createUserDB();

    daemon.addToStartup();
    http.start_API(home_server);

    std::thread logic_thread([&home_server]() {
        home_server.Watchlog();
    });

    std::cout << "Server system started." << std::endl;
 
    if (logic_thread.joinable())  logic_thread.join();

    return 0;
}