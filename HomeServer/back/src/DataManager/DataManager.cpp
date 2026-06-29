#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include "DataManager.h"

DataManager::DataManager() {
    this->login_db = "home_server";
    this->password_db = "psql";
    this->db_name = "home_server_db";
    this->table_name = "test_table";
    configuration("../conf/settings.conf");

    if (!startDB()) {
        std::cout << "error with starting db..." << std::endl;
    }
}

DataManager::~DataManager(){ }


void DataManager::configuration(std::string path_to_conf) {
    // db_login=home_server
    // db_open_password=psql
    // TODO    
}

bool DataManager::startDB() {
    if (!createUserDB(login_db, password_db)) {
        std::cout << "error with creating user: " + login_db << std::endl;
        return false;
    }
    if (!createtable(table_name)) {
        std::cout << "error with creating table: " + table_name << std::endl;
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
            
            // 1. Перевірка та створення користувача
            pqxx::result check_user = nt.exec("SELECT 1 FROM pg_roles WHERE rolname = " + nt.quote(login));
            if (check_user.empty()) {
                std::cout << "[DB Bootstrap] User " << login << " does not exist. Creating..." << std::endl;
                nt.exec("CREATE USER " + nt.sanitize_ident(login) + " WITH PASSWORD " + nt.quote(password) + ";");
                nt.exec("ALTER USER " + nt.sanitize_ident(login) + " WITH SUPERUSER;");
            } else {
                std::cout << "[DB Bootstrap] User " << login << " already exists." << std::endl;
            }

            // 2. Перевірка та створення БАЗИ ДАНИХ (тепер використовуємо db_name)
            pqxx::result check_db = nt.exec("SELECT 1 FROM pg_database WHERE datname = " + nt.quote(db_name));
            if (check_db.empty()) {
                std::cout << "[DB Bootstrap] Database '" + db_name + "' does not exist. Creating..." << std::endl;
                nt.exec("CREATE DATABASE " + nt.sanitize_ident(db_name) + " OWNER " + nt.sanitize_ident(login) + ";");
            } else {
                std::cout << "[DB Bootstrap] Database '" + db_name + "' already exists." << std::endl;
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
            tx.exec("CREATE TABLE IF NOT EXISTS " + tx.sanitize_ident(value_table_name) + " (id SERIAL PRIMARY KEY, data TEXT);");
            
            tx.commit();
            std::cout << "[DB] Table '" + value_table_name + "' is ready." << std::endl;
            return true;
        }
    } catch (const std::exception &e) {
        std::cerr << "[DB] Error creating table: " << e.what() << std::endl;
    }
    return false;
}