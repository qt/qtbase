# Generated from streambookmarks.pro.

cmake_minimum_required(VERSION 3.14)
project(streambookmarks LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/xml/streambookmarks")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Xml)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(streambookmarks
    main.cpp
    mainwindow.cpp mainwindow.h
    xbelreader.cpp xbelreader.h
    xbelwriter.cpp xbelwriter.h
)
target_link_libraries(streambookmarks PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Xml
)

install(TARGETS streambookmarks
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
