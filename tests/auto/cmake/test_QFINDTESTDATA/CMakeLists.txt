
cmake_minimum_required(VERSION 2.8.11)

project(test_QFINDTESTDATA)

find_package(Qt5Test REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(tests)
