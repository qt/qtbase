# Generated from fridgemagnets.pro.

cmake_minimum_required(VERSION 3.14)
project(fridgemagnets LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(fridgemagnets WIN32 MACOSX_BUNDLE
    draglabel.cpp draglabel.h
    dragwidget.cpp dragwidget.h
    fridgemagnets.qrc
    main.cpp
)
target_link_libraries(fridgemagnets PUBLIC
    Qt::Widgets
)

install(TARGETS fridgemagnets
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
