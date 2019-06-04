# Generated from mandelbrot.pro.

cmake_minimum_required(VERSION 3.14)
project(mandelbrot LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(mandelbrot WIN32 MACOSX_BUNDLE
    main.cpp
    mandelbrotwidget.cpp mandelbrotwidget.h
    renderthread.cpp renderthread.h
)
target_link_libraries(mandelbrot PUBLIC
    Qt::Widgets
)
install(TARGETS mandelbrot
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
