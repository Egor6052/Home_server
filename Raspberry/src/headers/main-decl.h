#ifndef MAIN_DECL_H
#define MAIN_DECL_H
#include <string>

extern std::string station_id;
extern std::string timestamp;
extern std::string temperature;
extern std::string latitude;
extern std::string longitude;
extern std::string humidity;

// void start_server();

// Getters
std::string getTimestamp();
std::string getLatitude();
std::string getLongitude();
std::string getTemperature();
std::string getHumidity();
std::string getStationID();

// UART
bool gpio_uart_init(const char* device);
bool usb_uart_init(const char *device);
bool uart_request_update();
void update_data_from_uart();

std::string getCurrentDateTime();

#endif

