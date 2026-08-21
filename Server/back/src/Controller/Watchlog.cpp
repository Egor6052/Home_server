#include "controller.h"
#include <thread>

void HomeServer::Watchlog() {
    while (uart2_fd < 0) {
        uart2_fd = initUART(STM32_UART_DEVICE);
        if (uart2_fd < 0) {
            std::cerr << "[UART2] Retrying in 2s...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    std::cout << "[UART2] uart2_fd=" << uart2_fd << " сразу после initUART()\n";
    std::cout << "Watchlog started on " << STM32_UART_DEVICE << " \n";

    while (true) {
        update_server_live_data();
        std::this_thread::sleep_for(std::chrono::seconds(15));
    }
}