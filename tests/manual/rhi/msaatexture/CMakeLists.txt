# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(msaatexture LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## msaatexture Binary:
#####################################################################

qt_internal_add_manual_test(msaatexture
    GUI
    SOURCES
        msaatexture.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set_source_files_properties("../shared/color.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.frag.qsb"
)
set_source_files_properties("../shared/color.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "color.vert.qsb"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set_source_files_properties("../shared/texture_ms4.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture_ms4.frag.qsb"
)
set(msaatexture_resource_files
    "../shared/color.frag.qsb"
    "../shared/color.vert.qsb"
    "../shared/texture.frag.qsb"
    "../shared/texture.vert.qsb"
    "../shared/texture_ms4.frag.qsb"
)

qt_internal_add_resource(msaatexture "msaatexture"
    PREFIX
        "/"
    FILES
        ${msaatexture_resource_files}
)
