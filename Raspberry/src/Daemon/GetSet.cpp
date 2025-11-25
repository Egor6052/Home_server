
#include <Daemon.h>
#include <iostream>
#include <string>


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
// }

// void Daemon::setBreakpoint(std::string valueBrreakpoint) {
//     this->breakpoint = valueBrreakpoint;
// }

// std::string Daemon::getBreakpoint() {
//     return this->breakpoint;
// }