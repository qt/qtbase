# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(treemodelcompleter LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(treemodelcompleter
    main.cpp
    mainwindow.cpp mainwindow.h
    treemodelcompleter.cpp treemodelcompleter.h
)

set_target_properties(treemodelcompleter PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(treemodelcompleter PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(treemodelcompleter_resource_files
    "resources/treemodel.txt"
)

qt_add_resources(treemodelcompleter "treemodelcompleter"
    PREFIX
        "/"
    FILES
        ${treemodelcompleter_resource_files}
)

install(TARGETS treemodelcompleter
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET treemodelcompleter
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
