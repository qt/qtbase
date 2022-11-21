# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(hellovulkancubes LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/vulkan/hellovulkancubes")

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
set_source_files_properties("../shared/block.buf"
    PROPERTIES QT_RESOURCE_ALIAS "block.buf"
)

set_source_files_properties("../shared/qt_logo.buf"
    PROPERTIES QT_RESOURCE_ALIAS "qt_logo.buf"
)

set(hellovulkancubes_resource_files
    "../shared/block.buf"
    "../shared/qt_logo.buf"
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
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
