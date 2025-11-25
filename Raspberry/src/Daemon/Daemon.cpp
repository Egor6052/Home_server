#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <filesystem>
#include <Daemon.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>

Daemon::Daemon() {
    absolutePath();
    setServicePath("/etc/systemd/system/home_server.service");
    setProgramPath(absolutePath() + "/build/home_server");
    setWorkingDirectory(absolutePath());
}
Daemon::~Daemon() {

}

std::string Daemon::getCurrentDateTime() {
    using namespace std::chrono;

    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);

    std::tm tm{};
    gmtime_r(&t, &tm); // UTC time

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

    return oss.str();
}