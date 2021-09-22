
cmake_minimum_required(VERSION 3.16)

project(test_concurrent_module)

find_package(Qt6Concurrent 6.0.0 REQUIRED)

add_executable(mainapp main.cpp)

target_link_libraries(mainapp PRIVATE Qt::Concurrent)
