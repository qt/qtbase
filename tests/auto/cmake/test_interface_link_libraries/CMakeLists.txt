
cmake_minimum_required(VERSION 2.8)

project(test_interface_link_libraries)

find_package(Qt5Gui REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

cmake_policy(SET CMP0022 NEW)

include(GenerateExportHeader)

add_library(somelib SHARED somelib.cpp)
generate_export_header(somelib)
set_property(TARGET somelib PROPERTY LINK_LIBRARIES Qt5::Gui)
set_property(TARGET somelib PROPERTY INTERFACE_LINK_LIBRARIES Qt5::Gui)

add_executable(mainexe main.cpp)
set_property(TARGET mainexe PROPERTY LINK_LIBRARIES somelib)
