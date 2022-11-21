# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(licensewizard LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/dialogs/licensewizard")

find_package(Qt6 REQUIRED COMPONENTS Core Gui PrintSupport Widgets)

qt_standard_project_setup()

qt_add_executable(licensewizard
    licensewizard.cpp licensewizard.h
    main.cpp
)

set_target_properties(licensewizard PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(licensewizard PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::PrintSupport
    Qt6::Widgets
)

# Resources:
set(licensewizard_resource_files
    "images/logo.png"
    "images/watermark.png"
)

qt_add_resources(licensewizard "licensewizard"
    PREFIX
        "/"
    FILES
        ${licensewizard_resource_files}
)

install(TARGETS licensewizard
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
