# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(dockwidgets LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/mainwindows/dockwidgets")

find_package(Qt6
    REQUIRED COMPONENTS Core Gui Widgets
    OPTIONAL_COMPONENTS PrintSupport
)

qt_standard_project_setup()

qt_add_executable(dockwidgets
    main.cpp
    mainwindow.cpp mainwindow.h
)

set_target_properties(dockwidgets PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(dockwidgets PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if (TARGET Qt6::PrintSupport)
    target_link_libraries(dockwidgets PRIVATE Qt6::PrintSupport)
endif()

# Resources:
set(dockwidgets_resource_files
    "images/new.png"
    "images/print.png"
    "images/save.png"
    "images/undo.png"
)

qt_add_resources(dockwidgets "dockwidgets"
    PREFIX
        "/"
    FILES
        ${dockwidgets_resource_files}
)

install(TARGETS dockwidgets
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
