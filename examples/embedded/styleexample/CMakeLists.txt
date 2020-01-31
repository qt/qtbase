# Generated from styleexample.pro.

cmake_minimum_required(VERSION 3.14)
project(styleexample LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(styleexample
    main.cpp
    stylewidget.cpp stylewidget.h stylewidget.ui
    styleexample.qrc # special case: add
)
target_link_libraries(styleexample PUBLIC
    Qt::Widgets
)
install(TARGETS styleexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
