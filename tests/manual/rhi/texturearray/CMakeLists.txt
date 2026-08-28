# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(texturearray LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(texturearray
    GUI
    SOURCES
        texturearray.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set_source_files_properties("../shared/texture_arr.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture_arr.vert.qsb"
)
set_source_files_properties("../shared/texture_arr.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture_arr.frag.qsb"
)
set(texturearray_resource_files
    "../shared/texture_arr.vert.qsb"
    "../shared/texture_arr.frag.qsb"
)

qt_internal_add_resource(texturearray "texturearray"
    PREFIX
        "/"
    FILES
        ${texturearray_resource_files}
)
