# Generated from framebufferobject2.pro.

cmake_minimum_required(VERSION 3.14)
project(framebufferobject2 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/opengl/legacy/framebufferobject2")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS OpenGL)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(framebufferobject2
    glwidget.cpp glwidget.h
    main.cpp
)
target_link_libraries(framebufferobject2 PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::Widgets
)


# Resources:
set(framebufferobject2_resource_files
    "cubelogo.png"
)

qt6_add_resources(framebufferobject2 "framebufferobject2"
    PREFIX
        "/res"
    FILES
        ${framebufferobject2_resource_files}
)

install(TARGETS framebufferobject2
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
