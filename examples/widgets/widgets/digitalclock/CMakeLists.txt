# Generated from digitalclock.pro.

cmake_minimum_required(VERSION 3.14)
project(digitalclock LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(digitalclock WIN32 MACOSX_BUNDLE
    digitalclock.cpp digitalclock.h
    main.cpp
)
target_link_libraries(digitalclock PUBLIC
    Qt::Widgets
)

install(TARGETS digitalclock
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
