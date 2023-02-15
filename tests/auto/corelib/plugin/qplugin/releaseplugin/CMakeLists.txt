# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## releaseplugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(releaseplugin
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../plugins"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
)

qt_autogen_tools_initial_setup(releaseplugin)
