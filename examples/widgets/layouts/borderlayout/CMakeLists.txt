# Generated from borderlayout.pro.

cmake_minimum_required(VERSION 3.14)
project(borderlayout LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(borderlayout
    borderlayout.cpp borderlayout.h
    main.cpp
    window.cpp window.h
)
target_link_libraries(borderlayout PUBLIC
    Qt::Widgets
)

install(TARGETS borderlayout
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
