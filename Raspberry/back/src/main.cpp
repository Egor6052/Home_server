#include <iostream>
#include <thread>
#include <chrono>
#include "Daemon/Daemon.h"
#include "server/Server.h"

int main() {
    Daemon daemon;
    daemon.addToStartup();
    Server server;

    std::thread logic_thread([&server]() { server.run_logic(); });

    while (true) { }

    return 0;
}