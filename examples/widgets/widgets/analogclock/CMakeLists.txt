# Generated from analogclock.pro.

cmake_minimum_required(VERSION 3.14)
project(analogclock LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(analogclock WIN32 MACOSX_BUNDLE
    analogclock.cpp analogclock.h
    main.cpp
)
target_link_libraries(analogclock PUBLIC
    Qt::Widgets
)

install(TARGETS analogclock
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
