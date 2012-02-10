
cmake_minimum_required(VERSION 2.8)

project(pass7)

find_package(Qt5Core REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})
add_definitions(${Qt5Core_DEFINITIONS})

add_executable(myobject main.cpp)
