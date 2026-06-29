#include "Daemon.h"
#include <iostream>

void Daemon::removeFromAutostart() {
    system("systemctl disable home_server.service");
    system("rm -f /etc/systemd/system/home_server.service");
    system("systemctl daemon-reload");

    std::cout << "\033[1m\033[32mDaemon has been removed from autostart!\033[0m" << std::endl;
}
