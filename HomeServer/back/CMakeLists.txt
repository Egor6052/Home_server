cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# Project name
project(home_server)

# Option to use static libraries
option(USE_STATIC "Use static library" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Display system message
message(STATUS "OS: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compile with static library? ${USE_STATIC}")

# ============ CURL (автоустановка перед find_package) ============
if (NOT WIN32)
    message(STATUS "CURL: Checking environment...")

    execute_process(
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install_curl.sh"
        RESULT_VARIABLE CURL_INSTALL_RESULT
    )

    if(CURL_INSTALL_RESULT EQUAL 0)
        message(STATUS "CURL: Ready.")
    else()
        message(FATAL_ERROR "CURL: Installation or check failed with exit code: ${CURL_INSTALL_RESULT}")
    endif()
endif()

# ============ nlohmann_json (автоустановка перед find_package) ============
if (NOT WIN32)
    message(STATUS "nlohmann_json: Checking environment...")

    execute_process(
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install_nlohmann_json.sh"
        RESULT_VARIABLE NLOHMANN_JSON_INSTALL_RESULT
    )

    if(NLOHMANN_JSON_INSTALL_RESULT EQUAL 0)
        message(STATUS "nlohmann_json: Ready.")
    else()
        message(FATAL_ERROR "nlohmann_json: Installation or check failed with exit code: ${NLOHMANN_JSON_INSTALL_RESULT}")
    endif()
endif()

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Path to source files
set(SOURCE_FILES
    # Server
    src/Server/Firebase.cpp
    src/Server/GetSet.cpp
    src/Server/MainLogic.cpp
    src/Server/Ping.cpp
    src/Server/Server.cpp
    src/Server/Uart.cpp
    src/Server/saveToFile.cpp
    src/Server/UsbCam.cpp
    src/Server/stm32_helper.cpp

    # Daemon
    src/Daemon/absolutePath.cpp
    src/Daemon/addToStartup.cpp
    src/Daemon/Daemon.cpp
    src/Daemon/GetSet.cpp
    src/Daemon/removeFromAutostart.cpp
    src/Daemon/NginxConfig.cpp

    # Http
    src/Http/Http.cpp
    src/Http/getDashIP.cpp
    src/Http/handle24data.cpp
    src/Http/handleCameraControl.cpp
    src/Http/handleLog.cpp
    src/Http/handleRestartDaemon.cpp
    

    # DataManager
    src/DataManager.cpp

)

# Додавання виконуваного файлу
add_executable(home_server
    src/main.cpp
    ${SOURCE_FILES}
)

# Компіляторні опції
target_compile_options(home_server
    PRIVATE
        -Wall -fPIC
        -Wno-psabi
)


# Підключення include-шляхів
target_include_directories(home_server
    PRIVATE
        # include
        src/Daemon
        src/server
        src/Logger
        src/DataManager
        
        ${CMAKE_CURRENT_BINARY_DIR}
)

# Для статичного лінкування (якщо USE_STATIC увімкнено)
if(USE_STATIC)
    if(UNIX AND NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif()

# add_executable(home_server src/main.cpp src/server/server.cpp) # Ваші файли

# ============ nlohmann/json (header-only) ============
target_include_directories(home_server PRIVATE lib/json)

# Лінккуємо бібліотеки
target_link_libraries(home_server PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)

# ============ OpenSSL (для Linux) ============

if (NOT WIN32)

    # Ищем заголовочный файл OpenSSL
    find_path(OPENSSL_INCLUDE_DIR
        NAMES openssl/ec.h
        PATHS /usr/include /usr/local/include
    )

    # Если не найден — запускаем скрипт установки
    if(NOT OPENSSL_INCLUDE_DIR)
        message(STATUS "OpenSSL headers not found. Running installation script...")

        execute_process(
            COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install_openssl.sh"
            RESULT_VARIABLE OPENSSL_SCRIPT_RESULT
        )

        if(OPENSSL_SCRIPT_RESULT EQUAL 0)
            message(STATUS "OpenSSL installed successfully. Searching again...")

            find_path(OPENSSL_INCLUDE_DIR
                NAMES openssl/ec.h
                PATHS /usr/include /usr/local/include
            )
        else()
            message(WARNING "OpenSSL installation script failed with code: ${OPENSSL_SCRIPT_RESULT}")
        endif()
    endif()

    # Финальная проверка
    if(OPENSSL_INCLUDE_DIR)
        message(STATUS "Found OpenSSL headers: ${OPENSSL_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "OpenSSL headers NOT FOUND and installation failed!")
    endif()
endif()

# ============ PostgreSQL ============
if (NOT WIN32)
    message(STATUS "PostgreSQL: Checking environment...")

    execute_process(
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install_postgres.sh"
        RESULT_VARIABLE PG_INSTALL_RESULT
    )

    if(PG_INSTALL_RESULT EQUAL 0)
        message(STATUS "PostgreSQL: Ready.")
    else()
        message(FATAL_ERROR "PostgreSQL: Installation or check failed with exit code: ${PG_INSTALL_RESULT}")
    endif()
endif()

# ============ Nginx ============
if (NOT WIN32)
    message(STATUS "Nginx: Checking environment...")

    execute_process(
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install_nginx.sh"
        RESULT_VARIABLE NGINX_INSTALL_RESULT
    )

    if(NGINX_INSTALL_RESULT EQUAL 0)
        message(STATUS "Nginx: Ready.")
    else()
        message(FATAL_ERROR "Nginx: Installation or check failed with exit code: ${NGINX_INSTALL_RESULT}")
    endif()
endif()
# ===================================