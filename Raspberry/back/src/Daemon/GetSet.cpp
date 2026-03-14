
#include "Daemon.h"
#include <iostream>
#include <string>


// void Daemon::setServerIP(std::string valueServerIP) {
//     this->serverIP = valueServerIP;
// }

// std::string Daemon::getServerIP() {
//     return serverIP;
// }

// void Daemon::setDaemon_on_off(bool state) {
//     std::lock_guard<std::mutex> lock(state_mutex);
//     daemon_on_off = state;
//     std::cout << "Daemon state set to: " << (state ? "ON" : "OFF") << std::endl;
// }

// bool Daemon::getDaemon_on_off() {
//     std::lock_guard<std::mutex> lock(state_mutex);
//     return daemon_on_off;
// }

void Daemon::setServicePath(std::string valuePath) {
    this->servicePath = valuePath;
}

std::string Daemon::getServicePath() {
    return this->servicePath;
}


void Daemon::setProgramPath(std::string valueProgramPath) {
    this->program_path = valueProgramPath;
}

std::string Daemon::getProgramPath() {
    return this->program_path;
}


void Daemon::setWorkingDirectory(std::string valueworkingDirectory) {
    this->working_directory = valueworkingDirectory;
}

std::string Daemon::getWorkingDirectory() {
    return this->working_directory;
}

// void Daemon::setPort(int valuePort) {
//     this->port = valuePort;
// }

// int Daemon::getPort() {
//     return this->port;
// // }

// void Daemon::setBreakpoint(std::string valueBrreakpoint) {
//     this->breakpoint = valueBrreakpoint;
// }

// std::string Daemon::getBreakpoint() {
//     return this->breakpoint;
// }