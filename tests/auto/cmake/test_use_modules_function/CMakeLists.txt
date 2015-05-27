
cmake_minimum_required(VERSION 2.8)

project(test_use_modules_function)

set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(two two.cpp)
add_executable(three three.cpp)

find_package(Qt5Core)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

qt5_use_modules(two Test)
qt5_use_modules(three Gui Test)
