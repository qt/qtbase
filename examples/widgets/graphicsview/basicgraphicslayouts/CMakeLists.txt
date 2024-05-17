# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(basicgraphicslayouts LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(basicgraphicslayouts
    layoutitem.cpp layoutitem.h
    main.cpp
    window.cpp window.h
)

set_target_properties(basicgraphicslayouts PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(basicgraphicslayouts PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(basicgraphicslayouts_resource_files
    "images/block.png"
)

qt_add_resources(basicgraphicslayouts "basicgraphicslayouts"
    PREFIX
        "/"
    FILES
        ${basicgraphicslayouts_resource_files}
)

install(TARGETS basicgraphicslayouts
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET basicgraphicslayouts
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
