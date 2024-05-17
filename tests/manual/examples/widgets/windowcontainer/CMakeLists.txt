# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(windowcontainer LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/windowcontainer")

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Widgets)

qt_standard_project_setup()

qt_add_executable(windowcontainer
    ../../opengl/openglwindow/openglwindow.cpp ../../opengl/openglwindow/openglwindow.h
    windowcontainer.cpp
)

set_target_properties(windowcontainer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(windowcontainer PRIVATE
    ../../opengl/openglwindow
)

target_link_libraries(windowcontainer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::Widgets
)

install(TARGETS windowcontainer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
