#include "Server.h"
#include <iostream>
#include <string>
#include <random>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
  #include <cstdio>
#else
  #include <cstdio>
  #include <sys/wait.h>
#endif

bool HomeServer::ping() {

    const std::string ip = "8.8.8.8";
    std::string command;

#ifdef _WIN32
    // Windows: 1 пакет, таймаут 1000 мс
    command = "ping -n 1 -w 1000 " + ip + " >nul 2>&1";

    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) return false;

    int exit_code = _pclose(pipe);

    // У Windows ping повертає 0 при успіху
    return (exit_code == 0);

#else
    // Linux/macOS: 1 пакет, таймаут 1 сек
    command = "ping -c 1 -W 1 " + ip + " >/dev/null 2>&1";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;

    int exit_code = pclose(pipe);

    if (exit_code == -1) return false;

    return (WEXITSTATUS(exit_code) == 0);
#endif
}
