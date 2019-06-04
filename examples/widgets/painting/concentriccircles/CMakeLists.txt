# Generated from concentriccircles.pro.

cmake_minimum_required(VERSION 3.14)
project(concentriccircles LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(concentriccircles WIN32 MACOSX_BUNDLE
    circlewidget.cpp circlewidget.h
    main.cpp
    window.cpp window.h
)
target_link_libraries(concentriccircles PUBLIC
    Qt::Widgets
)

install(TARGETS concentriccircles
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
