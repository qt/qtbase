# Generated from qopenglwindow.pro.

cmake_minimum_required(VERSION 3.14)
project(qopenglwindow LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/opengl/qopenglwindow")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS OpenGL)

qt_add_executable(qopenglwindow
    background_renderer.cpp background_renderer.h
    main.cpp
)
set_target_properties(qopenglwindow PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(qopenglwindow PUBLIC
    .
)

target_link_libraries(qopenglwindow PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
)


# Resources:
set(shaders_resource_files
    "background.frag"
)

qt6_add_resources(qopenglwindow "shaders"
    PREFIX
        "/"
    FILES
        ${shaders_resource_files}
)

install(TARGETS qopenglwindow
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
