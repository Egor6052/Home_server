#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <map>

struct context_blackBox {
    bool flag_new_data;
    std::string alert;
};

class BlackBox {
private:

    std::string configured_log_path;
    std::mutex path_mtx;

    std::mutex mtx;
    std::condition_variable cv;
    std::queue<std::string> log_queue;
    std::atomic<bool> running{false};
    std::thread blackBox_thread;

    void blackBox();
    std::string generateLogFileName();
    std::string getLogsDirectory();
    void loadLogPathFromConfig();
    bool isNullSink(const std::string& path);
    void ensureLogsDirectoryExists();

    std::map<std::string, std::string> readConfigFile(const std::string& path);
    bool writeConfigFile(const std::string& path,
                          const std::map<std::string, std::string>& values,
                          std::string& errorMessage);

    // Singleton: приватний конструктор, копіювання заборонене
    BlackBox();

public:
    static BlackBox& instance();

    BlackBox(const BlackBox&) = delete;
    BlackBox& operator=(const BlackBox&) = delete;

    ~BlackBox();

    std::string getPathToFile();
    bool fileExist(const std::string& path);
    bool createNewBlackBoxFile(const std::string& path);
    void startBlackBox_worker();
    void stopBlackBox();
    void pushToBlackBox(const std::string& alert);
    std::string getCurrentTimeString();

    bool isPathWritable(const std::string& path);
    std::string getConfiguredLogPath();
    bool setConfiguredLogPath(const std::string& newPath, std::string& errorMessage);

    bool isSafeFilename(const std::string& filename);
    std::vector<std::string> listLogFiles();
    bool readLogFile(const std::string& filename, std::string& content, std::string& errorMessage);
    bool removeFile(const std::string& filename, std::string& errorMessage);

};