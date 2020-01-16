# Generated from overpainting.pro.

cmake_minimum_required(VERSION 3.14)
project(overpainting LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/opengl/legacy/overpainting")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS OpenGL)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(overpainting
    ../shared/qtlogo.cpp ../shared/qtlogo.h
    bubble.cpp bubble.h
    glwidget.cpp glwidget.h
    main.cpp
)
target_include_directories(overpainting PUBLIC
    ../shared
)

target_link_libraries(overpainting PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::Widgets
)

install(TARGETS overpainting
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
