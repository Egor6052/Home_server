#include <iostream>

class DataManager {
    private:

    std::string login_db;
    std::string password_db;
    std::string db_name;
    std::string table_name;

    public:

    DataManager();
    ~DataManager();

    // read .conf file, and fill the fields;
    void configuration(std::string path_to_conf);

    // create user, create tables;
    bool startDB();

    bool createUserDB(std::string login, std::string password);
    bool createtable(std::string value_table_name);
    // drop all tables;
    bool delete_all_data();

    // static bool isValidUserId(long long userId);
    // static bool isNotEmpty(const std::string &value);


};