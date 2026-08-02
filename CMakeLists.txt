cmake_minimum_required(VERSION 3.20)
project(EquitiesScreener)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(EquitiesScreener
        main.cpp
        data/Equity.cpp
        data/Equity.h
        engine/Engine.cpp
        engine/Engine.h)

target_include_directories(EquitiesScreener PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
