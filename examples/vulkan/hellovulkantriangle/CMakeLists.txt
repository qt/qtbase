# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(hellovulkantriangle LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

qt_standard_project_setup()

qt_add_executable(hellovulkantriangle
    ../shared/trianglerenderer.cpp ../shared/trianglerenderer.h
    main.cpp
)

set_target_properties(hellovulkantriangle PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(hellovulkantriangle PRIVATE
    Qt6::Core
    Qt6::Gui
)

# Resources:
set_source_files_properties("../shared/color_frag.spv"
    PROPERTIES QT_RESOURCE_ALIAS "color_frag.spv"
)

set_source_files_properties("../shared/color_vert.spv"
    PROPERTIES QT_RESOURCE_ALIAS "color_vert.spv"
)

set(hellovulkantriangle_resource_files
    "../shared/color_frag.spv"
    "../shared/color_vert.spv"
)

qt_add_resources(hellovulkantriangle "hellovulkantriangle"
    PREFIX
        "/"
    FILES
        ${hellovulkantriangle_resource_files}
)

install(TARGETS hellovulkantriangle
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET hellovulkantriangle
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
