# Generated from basicgraphicslayouts.pro.

cmake_minimum_required(VERSION 3.14)
project(basicgraphicslayouts LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(basicgraphicslayouts
    basicgraphicslayouts.qrc
    layoutitem.cpp layoutitem.h
    main.cpp
    window.cpp window.h
)
target_link_libraries(basicgraphicslayouts PUBLIC
    Qt::Widgets
)

install(TARGETS basicgraphicslayouts
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
