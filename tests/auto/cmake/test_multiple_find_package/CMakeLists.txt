
cmake_minimum_required(VERSION 2.8)

project(test_multiple_find_package)

find_package(Qt5Core REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_subdirectory(subdir1)

add_executable(exe1 main.cpp)
target_link_libraries(exe1 Qt5::Core)
