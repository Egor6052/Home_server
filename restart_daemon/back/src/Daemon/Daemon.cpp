#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <filesystem>
#include "Daemon.h"
#include "../lib/http/httplib.h"
#include <string>

Daemon::Daemon() {
    // daemon_on_off = false;
    absolutePath();
    setServicePath("/etc/systemd/system/restart_daemon.service");
    setProgramPath(absolutePath() + "/build/restart_daemon");
    setWorkingDirectory(absolutePath());
    // setServerIP("boat8m");
    // setPort(6060);
    // setBreakpoint("/daemon");
}

Daemon::~Daemon() {
    // if (http_thread.joinable()) {
    //     s.stop();
    //     http_thread.join();
    // }
}

// void Daemon::listen() {
//     if (http_thread.joinable()) {
//         http_thread.join();
//     }
// }

// void Daemon::stopHTTP() {
//      if (http_thread.joinable()) {
//         s.stop();
//         http_thread.join();
//     }
// }