# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(rhiwidgetproto LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(rhiwidgetproto
    GUI
    SOURCES
        examplewidget.cpp examplewidget.h
        rhiwidget.cpp rhiwidget.h rhiwidget_p.h
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

set_source_files_properties("../shared/texture.vert.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.vert.qsb"
)
set_source_files_properties("../shared/texture.frag.qsb"
    PROPERTIES QT_RESOURCE_ALIAS "texture.frag.qsb"
)
set(rhiwidgetproto_resource_files
    "../shared/texture.vert.qsb"
    "../shared/texture.frag.qsb"
)

qt_internal_add_resource(rhiwidgetproto "rhiwidgetproto"
    PREFIX
        "/"
    FILES
        ${rhiwidgetproto_resource_files}
)
