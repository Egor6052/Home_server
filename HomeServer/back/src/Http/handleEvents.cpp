#include "Http.h"
#include <thread>
#include <chrono>
#include <algorithm>

// Тримає HTTP-з'єднання відкритим і реєструє sink у stream_clients.
// Дані в sink пишуться асинхронно з інших потоків через broadcast_event()
// (напр. з handleCameraControl після старту/стопу камери).
void Http::handleEvents(const httplib::Request& req, httplib::Response& res) {
    Http::addCorsHeaders(res);
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");

    res.set_chunked_content_provider(
        "text/event-stream",
        [this](size_t /*offset*/, httplib::DataSink& sink) {
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                stream_clients.push_back(&sink);
            }

            // Блокуємо цей робочий потік, поки клієнт не відключиться.
            while (sink.is_writable()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                stream_clients.erase(
                    std::remove(stream_clients.begin(), stream_clients.end(), &sink),
                    stream_clients.end());
            }
            return true;
        });
}