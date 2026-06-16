# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(hellovulkancubes LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(hellovulkancubes
    camera.cpp camera.h
    main.cpp
    mainwindow.cpp mainwindow.h
    mesh.cpp mesh.h
    renderer.cpp renderer.h
    shader.cpp shader.h
    vulkanwindow.cpp vulkanwindow.h
)

set_target_properties(hellovulkancubes PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(hellovulkancubes PRIVATE
    Qt6::Concurrent
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(hellovulkancubes_resource_files
    "block.buf"
    "qt_logo.buf"
    "color_frag.spv"
    "color_phong_frag.spv"
    "color_phong_vert.spv"
    "color_vert.spv"
)

qt6_add_resources(hellovulkancubes "hellovulkancubes"
    PREFIX
        "/"
    FILES
        ${hellovulkancubes_resource_files}
)

install(TARGETS hellovulkancubes
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET hellovulkancubes
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
