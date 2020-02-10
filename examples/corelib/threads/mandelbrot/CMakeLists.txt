# Generated from mandelbrot.pro.

cmake_minimum_required(VERSION 3.14)
project(mandelbrot LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/corelib/threads/mandelbrot")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(mandelbrot
    main.cpp
    mandelbrotwidget.cpp mandelbrotwidget.h
    renderthread.cpp renderthread.h
)
target_link_libraries(mandelbrot PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

if(UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS)
    target_link_libraries(mandelbrot PUBLIC
        m
    )
endif()

install(TARGETS mandelbrot
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
