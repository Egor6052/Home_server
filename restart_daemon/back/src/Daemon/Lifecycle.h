#pragma once
#include <atomic>
#include <csignal>
#include <functional>
#include <vector>

class Lifecycle {
private:
    // Обробник сигналу в С може бути лише вільною/статичною функцією,
    // тому прапорець теж статичний - на процес завжди має бути один Lifecycle
    static std::atomic<bool> running;

    std::vector<std::function<void()>> cleanup_hooks;

    static void handleSignal(int signum);

public:
    Lifecycle();
    ~Lifecycle();
    static bool isRunning();

    Lifecycle(const Lifecycle&) = delete;
    Lifecycle& operator=(const Lifecycle&) = delete;

    // Реєструє дію, яка виконається один раз при штатному завершенні
    void onShutdown(std::function<void()> hook);

    // Блокує виконання, поки не прийде SIGINT/SIGTERM
    void waitForShutdown(int poll_ms = 200);

    // Викликає всі зареєстровані cleanup-дії по черзі
    void runCleanup();
};