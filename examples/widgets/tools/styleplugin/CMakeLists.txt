# Generated from styleplugin.pro.

# special case begin
cmake_minimum_required(VERSION 3.14)
project(styleplugin LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)
# special case end

add_subdirectory(stylewindow)
add_subdirectory(plugin)
