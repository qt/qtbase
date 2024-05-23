# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## TestIconEngine Plugin:
#####################################################################

qt_internal_add_plugin(TestIconPlugin
    STATIC
    OUTPUT_NAME qtesticonplugin
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    SKIP_INSTALL
    PLUGIN_TYPE iconengines
    DEFAULT_IF TRUE
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
)
