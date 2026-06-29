#include <deque>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <regex>

class LogBuffer {
private:
    std::deque<std::string> lines;
    const size_t max_lines = 150;
    std::mutex mtx;

    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::tm bt{};
#ifdef _WIN32
        localtime_s(&bt, &in_time_t);
#else
        localtime_r(&in_time_t, &bt);
#endif
        std::stringstream ss;
        ss << "[" << std::put_time(&bt, "%Y-%m-%d %H:%M:%S") << "] ";
        return ss.str();
    }

public:
    std::string strip_ansi(std::string text) {
        static const std::regex ansi_re("\x1b\\[[0-9;]*[mK]");
        return std::regex_replace(text, ansi_re, "");
    }

    void add_log(const std::string& raw_message) {
        if (raw_message.empty()) return;

        std::lock_guard<std::mutex> lock(mtx);
        std::stringstream ss(raw_message);
        std::string line;
        
        while (std::getline(ss, line)) {
            // Очищуємо від символу повернення каретки (якщо є)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // Очищуємо від ANSI кодів (кольорів)
            std::string clean_line = strip_ansi(line);

            // Ігноруємо зовсім порожні рядки, щоб не забивати лог
            if (clean_line.empty() || clean_line == "\n" || clean_line == "\r") {
                continue;
            }
            
            // Форматуємо: [час] текст
            std::string formatted_line = get_timestamp() + clean_line;
            
            lines.push_back(formatted_line);
            
            // Контролюємо розмір буфера
            if (lines.size() > max_lines) {
                lines.pop_front();
            }
        }
    }

    nlohmann::json get_all_logs_json() {
        std::lock_guard<std::mutex> lock(mtx);
        nlohmann::json j = nlohmann::json::array();
        for (const auto& line : lines) {
            j.push_back(line);
        }
        return j;
    }
};

extern LogBuffer global_logger;

// Клас для перехоплення std::cout
class LogStream : public std::stringbuf {
public:
    int sync() override {
        std::string s = str();
        if (!s.empty()) {
            global_logger.add_log(s);
            std::printf("%s", s.c_str());
            std::fflush(stdout);
            str("");
        }
        return 0;
    }
};