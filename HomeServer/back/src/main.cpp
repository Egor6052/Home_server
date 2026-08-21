#include <iostream>
#include <thread>
#include <chrono>
#include "Daemon/Daemon.h"
#include "Server/Server.h"
#include "Http/Http.h"
#include "DataManager/IDataManager.h"
#include "DataManager/DataManager.h"

int main() {

    Daemon daemon;
    Http http;
    HomeServer home_server;
    IDataManager *datamanager = new DataManager();
    home_server.db_manager(*datamanager);


    // daemon.addToStartup();
    // http.start_API(home_server);

    std::cout << "Server system started." << std::endl;

    return 0;
}