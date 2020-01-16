# Generated from pbuffers2.pro.

cmake_minimum_required(VERSION 3.14)
project(pbuffers2 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/opengl/legacy/pbuffers2")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS OpenGL)
find_package(Qt6 COMPONENTS Svg)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(pbuffers2
    glwidget.cpp glwidget.h
    main.cpp
)
target_link_libraries(pbuffers2 PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::Svg
    Qt::Widgets
)


# Resources:
set(pbuffers2_resource_files
    "bubbles.svg"
    "designer.png"
)

qt6_add_resources(pbuffers2 "pbuffers2"
    PREFIX
        "/res"
    FILES
        ${pbuffers2_resource_files}
)

install(TARGETS pbuffers2
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
