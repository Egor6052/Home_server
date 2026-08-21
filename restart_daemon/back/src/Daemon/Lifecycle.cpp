#include "Lifecycle.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>

std::atomic<bool> Lifecycle::running{true};

void Lifecycle::handleSignal(int signum) {
    running.store(false, std::memory_order_relaxed);
}

Lifecycle::Lifecycle() {
    struct sigaction sa{};
    sa.sa_handler = Lifecycle::handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

Lifecycle::~Lifecycle() {}


bool Lifecycle::isRunning() {
    return running.load(std::memory_order_relaxed);
}

void Lifecycle::onShutdown(std::function<void()> hook) {
    cleanup_hooks.push_back(std::move(hook));
}

void Lifecycle::waitForShutdown(int poll_ms) {
    while (running.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
    }
}

void Lifecycle::runCleanup() {
    std::cout << "\nShutting down gracefully...\n";

    for (auto& hook : cleanup_hooks) {
        try {
            hook();
        } catch (const std::exception& e) {
            std::cerr << "[Lifecycle] Cleanup hook threw: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[Lifecycle] Cleanup hook threw unknown exception" << std::endl;
        }
    }

    std::cout << "Shutdown complete.\n";
}