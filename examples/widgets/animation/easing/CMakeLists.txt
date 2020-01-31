# Generated from easing.pro.

cmake_minimum_required(VERSION 3.14)
project(easing LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(easing
    animation.h
    easing.qrc
    form.ui
    main.cpp
    window.cpp window.h
)
target_link_libraries(easing PUBLIC
    Qt::Widgets
)

install(TARGETS easing
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
