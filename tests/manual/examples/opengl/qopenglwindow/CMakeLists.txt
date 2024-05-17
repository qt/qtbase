# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qopenglwindow LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/opengl/qopenglwindow")

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL)

qt_standard_project_setup()

qt_add_executable(qopenglwindow
    background_renderer.cpp background_renderer.h
    main.cpp
)

set_target_properties(qopenglwindow PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(qopenglwindow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
)

# Resources:
set(shaders_resource_files
    "background.frag"
)

qt_add_resources(qopenglwindow "shaders"
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
