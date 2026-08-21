cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
 
# Project name
project(restart_daemon)
 
# Option to use static libraries
option(USE_STATIC "Use static library" ON)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
# Display system message
message(STATUS "OS: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compile with static library? ${USE_STATIC}")
 
 
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

    # Daemon
    src/Daemon/absolutePath.cpp
    src/Daemon/addToStartup.cpp
    src/Daemon/Daemon.cpp
    src/Daemon/GetSet.cpp
    src/Daemon/removeFromAutostart.cpp
    src/Daemon/Lifecycle.cpp
    src/Daemon/removeFromAutostart.cpp

    # Http
    src/Http/Http.cpp
    src/Http/handleBlackBoxSettings.cpp
    src/Http/handleConfig.cpp
    src/Http/handleDownloadBlackBoxFile.cpp
    src/Http/handleGetAllBlackBoxFiles.cpp
    src/Http/handleRemoveBlackBoxFile.cpp
    src/Http/handleControlSTM.cpp
    src/Http/handleStmSettings.cpp
    
    # RestartHelper
    src/RestartHelper/stm32_helper.cpp

    # BlackBox
    src/BlackBox/BlackBox.cpp

)

# Додавання виконуваного файлу
add_executable(restart_daemon
    src/main.cpp
    ${SOURCE_FILES}
)

# Компіляторні опції
target_compile_options(restart_daemon
    PRIVATE
        -Wall -fPIC
        -Wno-psabi
)


# Підключення include-шляхів
target_include_directories(restart_daemon
    PRIVATE
        # include
        src/Daemon
        src/Http
        src/RestartHelper
        src/BlackBox
        
        ${CMAKE_CURRENT_BINARY_DIR}
)

# Для статичного лінкування (якщо USE_STATIC увімкнено)
if(USE_STATIC)
    if(UNIX AND NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif()

# add_executable(restart_daemon src/main.cpp src/server/server.cpp) # Ваші файли
 
# ============ nlohmann/json (header-only) ============
target_include_directories(restart_daemon PRIVATE lib/json)
 
# Лінккуємо бібліотеки
target_link_libraries(restart_daemon PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)
 
