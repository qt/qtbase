# Generated from tablet.pro.

cmake_minimum_required(VERSION 3.14)
project(qttablet LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(qttablet
    images.qrc
    main.cpp
    mainwindow.cpp mainwindow.h
    tabletapplication.cpp tabletapplication.h
    tabletcanvas.cpp tabletcanvas.h
)
target_link_libraries(qttablet PUBLIC
    Qt::Widgets
)

install(TARGETS qttablet
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
