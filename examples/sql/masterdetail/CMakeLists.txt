# Generated from masterdetail.pro.

cmake_minimum_required(VERSION 3.14)
project(masterdetail LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Sql)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Xml)

add_qt_gui_executable(masterdetail
    database.h
    dialog.cpp dialog.h
    main.cpp
    mainwindow.cpp mainwindow.h
    masterdetail.qrc
)
target_link_libraries(masterdetail PUBLIC
    Qt::Sql
    Qt::Widgets
    Qt::Xml
)

install(TARGETS masterdetail
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
