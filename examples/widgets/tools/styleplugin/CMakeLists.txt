cmake_minimum_required(VERSION 3.16)
project(styleplugin LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/styleplugin")

find_package(Qt6 COMPONENTS Widgets)

add_subdirectory(stylewindow)
add_subdirectory(plugin)
