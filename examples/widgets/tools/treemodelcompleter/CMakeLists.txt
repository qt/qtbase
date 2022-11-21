# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(treemodelcompleter LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/treemodelcompleter")

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
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
