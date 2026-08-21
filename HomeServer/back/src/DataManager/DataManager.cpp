#include <iostream>
#include <fstream>
#include <string>
#include <pqxx/pqxx>
#include "DataManager.h"

DataManager::DataManager() {
    configuration("../conf/settings.conf");
    startDB();

    if (!startDB()) {
        std::cout << "error with starting db..." << std::endl;
    }

    ANSI_RESET = "\u001B[0m";
    ANSI_BLACK = "\u001B[30m";
    ANSI_RED = "\u001B[31m";
    ANSI_YELLOW = "\u001B[33m";
    ANSI_BLUE = "\u001B[34m";
    ANSI_WHITE = "\u001B[37m";
    ANSI_GREEN = "\u001B[32m";

}

DataManager::~DataManager(){ }

void DataManager::configuration(std::string path_to_conf) {
    std::ifstream file(path_to_conf);
    if (!file.is_open()) {
        std::cerr << "[Config] Unable to open config file: " << path_to_conf << std::endl;
        return;
    }

    auto trim = [](std::string value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return std::string{};
        }
        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    };

    std::string line;
    while (std::getline(file, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        if (key == "db_login") {
            login_db = value;
        } else if (key == "db_open_password") {
            password_db = value;
        } else if (key == "db_name") {
            db_name = value;
        } else if (key == "table_name") {
            table_name = value;
        } else if (key == "FIREBASE_URL") {
            FIREBASE_URL = value;
        }
    }
}

bool DataManager::startDB() {
    if (!createUserDB(login_db, password_db)) {
        std::cout << "error with creating user: " << login_db << std::endl;
        return false;
    }
    if (!createtable(table_name)) {
        std::cout << "error with creating table: " << table_name << std::endl;
        return false;
    }
    return true;
}

bool DataManager::createUserDB(std::string login, std::string password) {
    try {
        // Підключаємося до дефолтної бази postgres під дефолтним юзером
        pqxx::connection bootstrap_conn("dbname=postgres user=postgres password=psql host=127.0.0.1");
        if (bootstrap_conn.is_open()) {
            pqxx::nontransaction nt(bootstrap_conn);

            // Перевірка та створення користувача
            pqxx::result check_user = nt.exec("SELECT 1 FROM pg_roles WHERE rolname = " + nt.quote(login));
            if (check_user.empty()) {
                std::cout << "[DB Bootstrap] User " << login << " does not exist. Creating..." << std::endl;
                nt.exec("CREATE USER " + nt.conn().quote_name(login) + " WITH PASSWORD " + nt.quote(password) + ";");
                nt.exec("ALTER USER " + nt.conn().quote_name(login) + " WITH SUPERUSER;");
            } else {
                std::cout << "[DB Bootstrap] User " << login << " already exists." << std::endl;
            }

            // Перевірка та створення БАЗИ ДАНИХ (тепер використовуємо db_name)
            pqxx::result check_db = nt.exec("SELECT 1 FROM pg_database WHERE datname = " + nt.quote(db_name));
            if (check_db.empty()) {
                std::cout << "[DB Bootstrap] Database '" << db_name << "' does not exist. Creating..." << std::endl;
                nt.exec("CREATE DATABASE " + nt.conn().quote_name(db_name) + " OWNER " + nt.conn().quote_name(login) + ";");
            } else {
                std::cout << "[DB Bootstrap] Database '" << db_name << "' already exists." << std::endl;
            }
            return true;
        }
    } catch (const std::exception &e) {
        std::cerr << "[DB Bootstrap] Critical error during initialization: " << e.what() << std::endl;
    }
    return false;
}

bool DataManager::createtable(std::string value_table_name) {
    try {
        // Підключаємося під новим юзером у нашу нову базу home_server_db
        pqxx::connection conn("dbname=" + db_name + " user=" + login_db + " password=" + password_db + " host=127.0.0.1");
        if (conn.is_open()) {
            pqxx::work tx(conn);
            // Створюємо саме ТАБЛИЦЮ всередині бази
            tx.exec("CREATE TABLE IF NOT EXISTS " + tx.conn().quote_name(value_table_name) + " (id SERIAL PRIMARY KEY, data TEXT);");
            tx.commit();
            std::cout << "[DB] Table '" << value_table_name << "' is ready." << std::endl;
            return true;
        }
    } catch (const std::exception &e) {
        std::cerr << "[DB] Error creating table: " << e.what() << std::endl;
    }
    return false;
}