
cmake_minimum_required(VERSION 2.8)

project(pass1)

set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(two two.cpp)
add_executable(three three.cpp)

find_package(Qt5Core)

qt5_use_modules(two Test)
qt5_use_modules(three Widgets Test)
