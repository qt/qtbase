# Generated from mainwindow.pro.

cmake_minimum_required(VERSION 3.14)
project(mainwindow LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(mainwindow
    colorswatch.cpp colorswatch.h
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.qrc
    toolbar.cpp toolbar.h
)
target_link_libraries(mainwindow PUBLIC
    Qt::Widgets
)

install(TARGETS mainwindow
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
