# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from invalidplugin.pro.

#####################################################################
## invalidplugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(invalidplugin
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../plugins"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:invalidplugin.pro:<TRUE>:
# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(invalidplugin)
