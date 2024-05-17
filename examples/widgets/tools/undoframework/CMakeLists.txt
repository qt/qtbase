# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(undoframework LANGUAGES CXX)

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
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET undoframework
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
