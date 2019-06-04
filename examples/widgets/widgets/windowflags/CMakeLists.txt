# Generated from windowflags.pro.

cmake_minimum_required(VERSION 3.14)
project(windowflags LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(windowflags WIN32 MACOSX_BUNDLE
    controllerwindow.cpp controllerwindow.h
    main.cpp
    previewwindow.cpp previewwindow.h
)
target_link_libraries(windowflags PUBLIC
    Qt::Widgets
)

install(TARGETS windowflags
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
