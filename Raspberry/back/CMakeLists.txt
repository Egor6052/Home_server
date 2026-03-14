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

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Path to source files
set(SOURCE_FILES
    # Server
    src/server/Firebase.cpp
    src/server/GetSet.cpp
    src/server/MainLogic.cpp
    src/server/Ping.cpp
    src/server/Server.cpp
    src/server/Uart.cpp

    # Daemon
    src/Daemon/absolutePath.cpp
    src/Daemon/addToStartup.cpp
    src/Daemon/Daemon.cpp
    src/Daemon/GetSet.cpp
    src/Daemon/removeFromAutostart.cpp
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


# Find CURL
# find_package(CURL REQUIRED)

# Add include directories
# target_include_directories(home_server PRIVATE ${CURL_INCLUDE_DIRS})


# We show the way to libraries
# find_library(WIRINGPI_LIBRARY wiringPi)
# target_link_libraries(home_server ${WIRINGPI_LIBRARY})