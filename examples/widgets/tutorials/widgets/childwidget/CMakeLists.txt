# Generated from childwidget.pro.

cmake_minimum_required(VERSION 3.14)
project(childwidget LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(childwidget
    main.cpp
)
target_link_libraries(childwidget PUBLIC
    Qt::Widgets
)

install(TARGETS childwidget
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
