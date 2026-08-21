#include <iostream>
#include "Daemon/Daemon.h"
#include "Daemon/Lifecycle.h"
#include "BlackBox/BlackBox.h"
#include "Http/Http.h"
#include "RestartHelper/Stm32RestartHelper.h"

int main() {
    Lifecycle lifecycle;
    Daemon daemon;
    Http http;

    daemon.addToStartup();

    BlackBox::instance().startBlackBox_worker();

    Helper restartHelper("/dev/ttyAMA0", 100);
    restartHelper.start();

    http.start_API(restartHelper);

    std::string startline = "Restart daemon is running!";
    std::cout << startline << std::endl;
    BlackBox::instance().pushToBlackBox(startline);

    lifecycle.onShutdown([&restartHelper]() { restartHelper.stop(); });
    lifecycle.onShutdown([]() { BlackBox::instance().stopBlackBox(); });

    lifecycle.waitForShutdown();
    lifecycle.runCleanup();

    return 0;
}