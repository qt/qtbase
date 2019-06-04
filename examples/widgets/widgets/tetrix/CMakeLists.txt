# Generated from tetrix.pro.

cmake_minimum_required(VERSION 3.14)
project(tetrix LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(tetrix WIN32 MACOSX_BUNDLE
    main.cpp
    tetrixboard.cpp tetrixboard.h
    tetrixpiece.cpp tetrixpiece.h
    tetrixwindow.cpp tetrixwindow.h
)
target_link_libraries(tetrix PUBLIC
    Qt::Widgets
)

install(TARGETS tetrix
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
