
cmake_minimum_required(VERSION 2.8)

project(test_multiple_find_package)

find_package(Qt5Core REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_subdirectory(subdir1)

add_executable(exe1 "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
include_directories(${Qt5Core_INCLUDE_DIRS})
