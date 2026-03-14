#include "Server.h"
#include <iostream>
#include <string>
#include <random>
#include <iomanip>
#include <sstream>

Server::Server() {

    uart_fd = -1;

    ANSI_RESET = "\u001B[0m";
    ANSI_BLACK = "\u001B[30m";
    ANSI_RED = "\u001B[31m";
    ANSI_YELLOW = "\u001B[33m";
    ANSI_BLUE = "\u001B[34m";
    ANSI_WHITE = "\u001B[37m";
    ANSI_GREEN = "\u001B[32m";


    const std::string FIREBASE_URL =
        "https://home-server-9e586-default-rtdb.firebaseio.com/measurements.json?auth=eOpdxQjllRN3hJ2Z9bIag33HIC2LaU97GkyBRvXG";

}

Server::~Server() {
    if (uart_fd >= 0) {
        close(uart_fd);
    }
}


bool Server::is_data_valid() {
    if (currentData.station_id.empty() || currentData.station_id == "abc") {
        return false;
    }
    
    if (currentData.lat == 0.0 && currentData.lon == 0.0 && currentData.temp == 0.0) {
        return false;
    }

    return true;
}

