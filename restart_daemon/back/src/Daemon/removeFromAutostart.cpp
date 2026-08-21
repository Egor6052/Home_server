#include "Daemon.h"

void Daemon::removeFromAutostart() {
    system("systemctl disable restart_daemon.service");
    system("rm -f /etc/systemd/system/restart_daemon.service");
    system("systemctl daemon-reload");

    std::cout << "\033[1m\033[32mDaemon has been removed from autostart!\033[0m" << std::endl;
}
