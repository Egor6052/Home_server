#include "BlackBox.h"
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <map>
#include <mutex>
#include <vector>
#include <thread>
#include <cstdlib>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <nlohmann/json.hpp>
#if __has_include(<filesystem>)
# include <filesystem>
namespace fs = std::filesystem;
#else
# include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

using json = nlohmann::json;

BlackBox::BlackBox() {
    loadLogPathFromConfig();
}

BlackBox::~BlackBox() {
    stopBlackBox();
}



std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') ||
         (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

BlackBox& BlackBox::instance() {
    static BlackBox instance_;
    return instance_;
}

void BlackBox::pushToBlackBox(const std::string& alert) {
    if (isNullSink(getLogsDirectory())) {
        return;
    }

    std::string time_str = getCurrentTimeString();
    std::string final_msg = "[" + time_str + "] " + alert;
    {
        std::lock_guard<std::mutex> lock(mtx);
        log_queue.push(final_msg);
    }
    cv.notify_one();
    // std::cout << "New data log to blackbox." << std::endl;
}

void BlackBox::startBlackBox_worker() {
    if (isNullSink(getLogsDirectory())) {
        std::cout << "[BlackBox] Logging disabled (null sink path configured)." << std::endl;
        return;
    }

    bool expected = false;
    if (running.compare_exchange_strong(expected, true)) {
        blackBox_thread = std::thread([this]() { this->blackBox(); });
    }
}

void BlackBox::blackBox() {
    ensureLogsDirectoryExists();

    std::string current_filename = generateLogFileName();
    bool file_already_existed = fileExist(current_filename);

    std::ofstream outfile(current_filename, std::ios::app);
    if (!outfile.is_open()) {
        std::cerr << "[BlackBox] Failed to open log file: " << current_filename << std::endl;
        return;
    }

    // Если файл за сегодня уже существовал (программа перезапустилась в течение того же дня),
    // один раз добавляем пустую строку-отступ, чтобы визуально отделить новый запуск.
    if (file_already_existed) {
        outfile << std::endl;
    }

    while (running.load(std::memory_order_relaxed)) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !log_queue.empty() || !running.load(std::memory_order_relaxed); });

        while (!log_queue.empty()) {
            std::string msg = log_queue.front();
            log_queue.pop();
            lock.unlock();

            std::string check_filename = generateLogFileName();
            bool name_changed = (check_filename != current_filename);
            // Тому перед кожним записом перевіряємо наявність файлу на диску.
            bool file_missing = !name_changed && !fileExist(current_filename);

            if (name_changed || file_missing) {
                outfile.close();
                current_filename = check_filename;
                outfile.open(current_filename, std::ios::app);
            }

            if (outfile.is_open()) {
                outfile << msg << std::endl;
            }

            lock.lock();
        }
    }

    if (outfile.is_open()) {
        outfile.close();
    }
}

void BlackBox::stopBlackBox() {
    running.store(false, std::memory_order_relaxed);
    cv.notify_one();
    if (blackBox_thread.joinable()) {
        blackBox_thread.join();
    }
}

std::string BlackBox::getPathToFile() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        auto path = fs::path(std::string(result, count));
        return path.parent_path().parent_path().string();
    } else {
        return "";
    }
}


std::map<std::string, std::string> BlackBox::readConfigFile(const std::string& path) {
    std::map<std::string, std::string> values;

    std::ifstream inFile(path);
    if (!inFile.is_open()) {
        return values;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        size_t eq_pos = trimmed.find('=');
        if (eq_pos == std::string::npos) {
            continue;
        }

        std::string key = trim(trimmed.substr(0, eq_pos));
        std::string value = stripQuotes(trim(trimmed.substr(eq_pos + 1)));

        if (!key.empty()) {
            values[key] = value;
        }
    }

    return values;
}

bool BlackBox::writeConfigFile(const std::string& path,
                                const std::map<std::string, std::string>& values,
                                std::string& errorMessage) {
    std::ofstream outFile(path);
    if (!outFile.is_open()) {
        errorMessage = "Failed to write config file: " + path;
        return false;
    }

    for (const auto& [key, value] : values) {
        outFile << key << "=\"" << value << "\"" << std::endl;
    }

    return true;
}


void BlackBox::loadLogPathFromConfig() {
    const std::string config_path = "../conf/blackbox.conf";

    auto values = readConfigFile(config_path);
    auto it = values.find("black_box_dir");

    if (it == values.end() || it->second.empty()) {
        std::cerr << "[BlackBox] Config file not found or key missing, falling back to auto-detected path: "
                  << config_path << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(path_mtx);
    configured_log_path = it->second;
}

bool BlackBox::isNullSink(const std::string& path) {
    // Розпізнаємо спеціальні файли-"чорні дірки", куди писати не потрібно
    return path == "/dev/null" || path == "/dev/zero";
}

bool BlackBox::isPathWritable(const std::string& path) {
    // null-sink завжди вважаємо "валідним" вибором (це свідоме вимкнення логування)
    if (isNullSink(path)) {
        return true;
    }

    // Пробуємо створити директорію (якщо її ще нема) і записати тестовий файл
    std::error_code ec;
    if (!fs::exists(path)) {
        if (!fs::create_directories(path, ec)) {
            return false;
        }
    }

    fs::path test_file = fs::path(path) / ".blackbox_write_test";
    std::ofstream test(test_file);
    bool writable = test.good();
    test.close();

    if (writable) {
        fs::remove(test_file, ec);
    }

    return writable;
}

std::string BlackBox::getLogsDirectory() {
    std::string base;
    {
        std::lock_guard<std::mutex> lock(path_mtx);
        base = configured_log_path;
    }

    // Якщо в конфізі заданий свій шлях - використовуємо його.
    // Якщо це null-sink (/dev/null і подібні) - повертаємо як є, без додавання "logs".
    if (!base.empty()) {
        if (isNullSink(base)) {
            return base;
        }
        fs::path logs_path = fs::path(base) / "logs";
        return logs_path.string();
    }

    // Інакше - старий механізм автовизначення:
    // на один рівень вище каталогу з виконуваним файлом ("back"), плюс підпапка logs
    fs::path base_path(getPathToFile());
    fs::path logs_path = base_path.parent_path() / "logs";
    return logs_path.string();
}

void BlackBox::ensureLogsDirectoryExists() {
    std::string logs_dir = getLogsDirectory();
    if (!fileExist(logs_dir)) {
        std::error_code ec;
        if (!fs::create_directories(logs_dir, ec)) {
            std::cerr << "[BlackBox] Failed to create logs directory: " << logs_dir
                      << " (" << ec.message() << ")" << std::endl;
        }
    }
}

bool BlackBox::fileExist(const std::string& path) {
    return fs::exists(path);
}

bool BlackBox::createNewBlackBoxFile(const std::string& path) {
    std::ofstream file(path);
    return file.good();
}

std::string BlackBox::generateLogFileName() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    char time_buf[50];
    // Формат имени: YYYY-MM-DD (будет оставаться одинаковым весь день)
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d", std::localtime(&now_c));

    fs::path file_path = fs::path(getLogsDirectory()) / ("black_box_" + std::string(time_buf) + ".log");
    return file_path.string();
}

std::string BlackBox::getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

std::string BlackBox::getConfiguredLogPath() {
    std::lock_guard<std::mutex> lock(path_mtx);
    if (!configured_log_path.empty()) {
        return configured_log_path;
    }
    // Якщо в конфізі нічого не задано - повертаємо автовизначений шлях,
    // щоб фронт показав реальне місце, куди зараз пишуться логи.
    return getPathToFile();
}

bool BlackBox::setConfiguredLogPath(const std::string& newPath, std::string& errorMessage) {
    if (newPath.empty()) {
        errorMessage = "Path cannot be empty";
        return false;
    }

    if (!isPathWritable(newPath)) {
        errorMessage = "Path is not writable: " + newPath;
        return false;
    }

    if (configured_log_path == newPath) {
        // Шлях не змінився - нічого зберігати і перезапускати не потрібно
        return false;
    }

    const std::string config_path = "../conf/blackbox.conf";

    auto values = readConfigFile(config_path);
    values["black_box_dir"] = newPath;

    if (!writeConfigFile(config_path, values, errorMessage)) {
        return false;
    }

    bool wasRunning = running.load(std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(path_mtx);
        configured_log_path = newPath;
    }

    bool isNowNullSink = isNullSink(getLogsDirectory());

    if (!wasRunning && !isNowNullSink) {
        startBlackBox_worker();
    } else if (wasRunning && isNowNullSink) {
        stopBlackBox();
    } else if (wasRunning && !isNowNullSink) {
        pushToBlackBox("Log path changed to: " + newPath);
    }

    return true;
}


std::vector<std::string> BlackBox::listLogFiles() {
    std::vector<std::string> files;
    std::string logs_dir = getLogsDirectory();
    if (isNullSink(logs_dir) || !fileExist(logs_dir)) {
        return files;
    }
    for (const auto& entry : fs::directory_iterator(logs_dir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().filename().string()); // только имя файла
        }
    }
    return files;
}


bool BlackBox::isSafeFilename(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }
    // Забороняємо слеші і ".." - ім'я має бути "голим" (без шляху),
    // щоб неможливо було вийти за межі директорії логів.
    if (filename.find('/') != std::string::npos ||
        filename.find('\\') != std::string::npos ||
        filename.find("..") != std::string::npos) {
        return false;
    }
    return true;
}


bool BlackBox::readLogFile(const std::string& filename, std::string& content, std::string& errorMessage) {
    if (!isSafeFilename(filename)) {
        errorMessage = "Invalid filename";
        return false;
    }
 
    std::string logs_dir = getLogsDirectory();
    if (isNullSink(logs_dir)) {
        errorMessage = "Logging is disabled (null sink configured)";
        return false;
    }
 
    fs::path file_path = fs::path(logs_dir) / filename;
 
    if (!fileExist(file_path.string())) {
        errorMessage = "File not found: " + filename;
        return false;
    }
 
    std::ifstream inFile(file_path.string());
    if (!inFile.is_open()) {
        errorMessage = "Failed to open file: " + filename;
        return false;
    }
 
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    content = buffer.str();
    inFile.close();
 
    return true;
}


bool BlackBox::removeFile(const std::string& filename, std::string& errorMessage) {
    if (!isSafeFilename(filename)) {
        errorMessage = "Invalid filename";
        return false;
    }
 
    std::string logs_dir = getLogsDirectory();
    if (isNullSink(logs_dir)) {
        errorMessage = "Logging is disabled (null sink configured)";
        return false;
    }
 
    fs::path file_path = fs::path(logs_dir) / filename;
 
    if (!fileExist(file_path.string())) {
        errorMessage = "File not found: " + filename;
        return false;
    }
 
    std::error_code ec;
    if (!fs::remove(file_path, ec)) {
        errorMessage = "Failed to remove file: " + filename +
                        (ec ? (" (" + ec.message() + ")") : "");
        return false;
    }
 
    return true;
}
 
