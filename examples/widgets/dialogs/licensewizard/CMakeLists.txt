# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(licensewizard LANGUAGES CXX)

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
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET licensewizard
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
