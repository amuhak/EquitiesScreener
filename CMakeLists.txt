cmake_minimum_required(VERSION 3.20)
project(EquitiesScreener)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(EquitiesScreener
        main.cpp
        data/Equity.cpp
        data/Equity.h
        engine/Engine.cpp
        engine/Engine.h
        io/Csv.cpp
        io/Csv.h
        config/Config.cpp
        config/Config.h)

target_include_directories(EquitiesScreener PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Framework-free unit tests for the settings-file parser
add_executable(EquitiesScreenerConfigTests
        config/Config_test.cpp
        data/Equity.cpp
        data/Equity.h
        engine/Engine.cpp
        engine/Engine.h
        config/Config.cpp
        config/Config.h)

target_include_directories(EquitiesScreenerConfigTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
