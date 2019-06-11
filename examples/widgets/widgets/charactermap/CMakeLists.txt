# Generated from charactermap.pro.

cmake_minimum_required(VERSION 3.14)
project(charactermap LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(charactermap WIN32 MACOSX_BUNDLE
    characterwidget.cpp characterwidget.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
target_link_libraries(charactermap PUBLIC
    Qt::Widgets
)

install(TARGETS charactermap
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
