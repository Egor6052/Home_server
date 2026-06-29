#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include "Daemon.h"
#include <string>
#include <filesystem>

#ifdef _WIN32
    include <windows.h>
#elif __APPLE__
    #include <mach-o/dyld.h>
#else
    #include <unistd.h>
#endif


std::string Daemon::absolutePath() {

#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        return "";
    }

    auto path = std::filesystem::path(std::string(buffer, len));
    return path.parent_path().parent_path().string();

#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count == -1) {
        return "";
    }

    auto path = std::filesystem::path(std::string(result, count));
    return path.parent_path().parent_path().string();
#endif
}