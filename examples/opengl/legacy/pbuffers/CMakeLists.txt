# Generated from pbuffers.pro.

cmake_minimum_required(VERSION 3.14)
project(pbuffers LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/opengl/legacy/pbuffers")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS OpenGL)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(pbuffers
    cube.cpp cube.h
    glwidget.cpp glwidget.h
    main.cpp
)
target_link_libraries(pbuffers PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::Widgets
)


# Resources:
set(pbuffers_resource_files
    "cubelogo.png"
)

qt6_add_resources(pbuffers "pbuffers"
    PREFIX
        "/res"
    FILES
        ${pbuffers_resource_files}
)

install(TARGETS pbuffers
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
