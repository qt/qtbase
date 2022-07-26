cmake_minimum_required(VERSION 3.16)
project(plugandpaint LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/plugandpaint")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_subdirectory(plugins)
add_subdirectory(app)
