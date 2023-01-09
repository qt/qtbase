# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(undoframework LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/undoframework")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(undoframework
    commands.cpp commands.h
    diagramitem.cpp diagramitem.h
    diagramscene.cpp diagramscene.h
    main.cpp
    mainwindow.cpp mainwindow.h
)

set_target_properties(undoframework PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(undoframework PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(undoframework_resource_files
    "icons/cross.png"
    "icons/rectangle.png"
    "icons/redo.png"
    "icons/remove.png"
    "icons/triangle.png"
    "icons/undo.png"
)

qt_add_resources(undoframework "undoframework"
    PREFIX
        "/"
    FILES
        ${undoframework_resource_files}
)

install(TARGETS undoframework
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
