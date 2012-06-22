
cmake_minimum_required(VERSION 2.8)

project(test_multiple_find_package)

find_package(Qt5Core REQUIRED)

add_subdirectory(subdir1)

