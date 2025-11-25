#pragma once

#include <iostream>
#include <string.h>

extern std::string ANSI_RESET;
extern std::string ANSI_BLACK;
extern std::string ANSI_RED;
extern std::string ANSI_YELLOW;
extern std::string ANSI_BLUE;
extern std::string ANSI_WHITE;
extern std::string ANSI_GREEN;


void connect_to_firebase();
void send_data_to_firebase(std::string& station_id, std::string timestamp, std::string& latitude, std::string& longitude, std::string& temperature, std::string& humidity);