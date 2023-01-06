# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(cube LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/opengl/cube")

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL OpenGLWidgets Widgets)

qt_standard_project_setup()

qt_add_executable(cube
    geometryengine.cpp geometryengine.h
    main.cpp
    mainwidget.cpp mainwidget.h
)

set_target_properties(cube PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(cube PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Widgets
)

# Resources:
set(shaders_resource_files
    "fshader.glsl"
    "vshader.glsl"
)

qt6_add_resources(cube "shaders"
    PREFIX
        "/"
    FILES
        ${shaders_resource_files}
)

set(textures_resource_files
    "cube.png"
)

qt6_add_resources(cube "textures"
    PREFIX
        "/"
    FILES
        ${textures_resource_files}
)

install(TARGETS cube
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
