# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(invalidplugin)
