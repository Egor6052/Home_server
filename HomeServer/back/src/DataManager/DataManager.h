#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../lib/http/httplib.h"
#include "IDataManager.h"

class DataManager : public IDataManager {
    private:
    std::string FIREBASE_URL;

    std::string login_db;
    std::string password_db;
    std::string db_name;
    std::string table_name;

    std::string ANSI_RESET;
    std::string ANSI_BLACK;
    std::string ANSI_RED;
    std::string ANSI_YELLOW;
    std::string ANSI_BLUE;
    std::string ANSI_WHITE;
    std::string ANSI_GREEN;


    public:

    DataManager();
    ~DataManager();

    // read .conf file, and fill the fields;
    void configuration(std::string path_to_conf);

    // create user, create tables;
    bool startDB() override;
    bool createUserDB(std::string login, std::string password);
    bool createtable(std::string value_table_name);

    // Firebase
    void connect_to_firebase();
    void send_data_to_firebase(std::string& station_id, std::string timestamp, std::string& latitude, std::string& longitude, std::string& temperature, std::string& humidity);

    // drop all tables;
    // bool delete_all_data();

    // static bool isValidUserId(long long userId);
    // static bool isNotEmpty(const std::string &value);


};