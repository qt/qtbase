# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(mainwindow LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/mainwindow")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(mainwindow
    colorswatch.cpp colorswatch.h
    main.cpp
    mainwindow.cpp mainwindow.h
    toolbar.cpp toolbar.h
)

set_target_properties(mainwindow PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(mainwindow PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(mainwindow_resource_files
    "qt.png"
    "titlebarCenter.png"
    "titlebarLeft.png"
    "titlebarRight.png"
)

qt_add_resources(mainwindow "mainwindow"
    PREFIX
        "/res"
    FILES
        ${mainwindow_resource_files}
)

install(TARGETS mainwindow
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
