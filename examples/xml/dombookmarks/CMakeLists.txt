# Generated from dombookmarks.pro.

cmake_minimum_required(VERSION 3.14)
project(dombookmarks LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Xml)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(dombookmarks
    main.cpp
    mainwindow.cpp mainwindow.h
    xbeltree.cpp xbeltree.h
)
target_link_libraries(dombookmarks PUBLIC
    Qt::Widgets
    Qt::Xml
)
install(TARGETS dombookmarks
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
