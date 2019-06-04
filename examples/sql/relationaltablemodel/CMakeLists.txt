# Generated from relationaltablemodel.pro.

cmake_minimum_required(VERSION 3.14)
project(relationaltablemodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Sql)
find_package(Qt5 COMPONENTS Widgets)

add_executable(relationaltablemodel WIN32 MACOSX_BUNDLE
    ../connection.h
    relationaltablemodel.cpp
)
target_link_libraries(relationaltablemodel PUBLIC
    Qt::Sql
    Qt::Widgets
)

install(TARGETS relationaltablemodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
