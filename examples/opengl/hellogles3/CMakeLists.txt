# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(hellogles3 LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/opengl/hellogles3")

find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL)

qt_standard_project_setup()

qt_add_executable(hellogles3
    ../hellogl2/logo.cpp ../hellogl2/logo.h
    glwindow.cpp glwindow.h
    main.cpp
)

set_target_properties(hellogles3 PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(hellogles3 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
)

# Resources:
set(hellogles3_resource_files
    "qtlogo.png"
)

qt_add_resources(hellogles3 "hellogles3"
    PREFIX
        "/"
    FILES
        ${hellogles3_resource_files}
)

install(TARGETS hellogles3
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
