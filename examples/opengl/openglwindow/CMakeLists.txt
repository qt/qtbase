# Generated from openglwindow.pro.

cmake_minimum_required(VERSION 3.14)
project(openglwindow LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/opengl/openglwindow")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS OpenGL)

add_qt_gui_executable(openglwindow
    main.cpp
    openglwindow.cpp openglwindow.h
)
target_include_directories(openglwindow PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(openglwindow PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
)

install(TARGETS openglwindow
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
