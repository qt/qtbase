# Generated from undo.pro.

cmake_minimum_required(VERSION 3.14)
project(undo LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(undo
    commands.cpp commands.h
    document.cpp document.h
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    undo.qrc
)
target_link_libraries(undo PUBLIC
    Qt::Widgets
)

install(TARGETS undo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
