#include "Server.h"
#include <fstream>
#include <filesystem>
#include <chrono>

#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

bool HomeServer::saveToFile() {
    try {
        
        if (!fs::exists(path_to_db)) {
            if (!fs::create_directories(path_to_db)) {
                std::cerr << "[Storage] Critical: Could not create directory " << path_to_db << std::endl;
                return false;
            }
        }

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_r(&t, &tm);

        std::ostringstream file_name;
        file_name << path_to_db << "/data_" << std::put_time(&tm, "%Y-%m") << ".json";
        std::string file_path = file_name.str();

        nlohmann::json new_entry;
        new_entry["id"]   = currentData.station_id;
        new_entry["lat"]  = currentData.lat;
        new_entry["lng"]  = currentData.lon;
        new_entry["temp"] = currentData.temp;
        new_entry["hum"]  = currentData.humidity;
        new_entry["time"] = static_cast<long long>(t); // Unix timestamp

        nlohmann::json file_data = nlohmann::json::array();
        
        if (fs::exists(file_path) && fs::file_size(file_path) > 0) {
            std::ifstream input_file(file_path);
            try {
                input_file >> file_data;
                if (!file_data.is_array()) {
                    file_data = nlohmann::json::array();
                }
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "[Storage] Parse error (file might be corrupted): " << e.what() << std::endl;
                file_data = nlohmann::json::array();
            }
            input_file.close();
        }

        file_data.push_back(new_entry);

        std::ofstream output_file(file_path);
        if (!output_file.is_open()) {
            std::cerr << "[Storage] Could not open file for writing: " << file_path << std::endl;
            return false;
        }

        output_file << file_data.dump() << std::endl;
        output_file.close();

        std::cout << "[Storage] Data saved to " << file_path << " (Unixtime: " << new_entry["time"] << ")" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[Storage] Exception: " << e.what() << std::endl;
        return false;
    }
}


nlohmann::json HomeServer::getLast24Records() {
    namespace fs = std::filesystem;
    
    // Отримуємо назву поточного файлу (як у saveToFile)
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    
    std::ostringstream file_name;
    file_name << path_to_db << "/data_" << std::put_time(&tm, "%Y-%m") << ".json";
    std::string file_path = file_name.str();

    nlohmann::json result = nlohmann::json::array();

    if (fs::exists(file_path)) {
        try {
            std::ifstream file(file_path);
            nlohmann::json full_data;
            file >> full_data;

            if (full_data.is_array()) {
                int total = full_data.size();
                int start = std::max(0, total - 24);
                
                for (int i = start; i < total; ++i) {
                    result.push_back(full_data[i]);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[Storage] Error reading last 24: " << e.what() << std::endl;
        }
    }
    return result;
}