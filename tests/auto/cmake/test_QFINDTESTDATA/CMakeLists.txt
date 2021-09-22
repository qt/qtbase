
cmake_minimum_required(VERSION 3.16)

project(test_QFINDTESTDATA)

find_package(Qt6Test REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(tests)
