cmake_minimum_required(VERSION 3.16)
project(plugandpaint LANGUAGES CXX)

find_package(Qt6 COMPONENTS Core Gui Widgets)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/echoplugin")

add_subdirectory(plugin)
add_subdirectory(echowindow)
