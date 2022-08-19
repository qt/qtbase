# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## moctestplugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(moctestplugin
    STATIC
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
    DEFINES
        QT_STATICPLUGIN
)

qt_internal_add_resource(moctestplugin "qmake_plugin_resource"
    PREFIX
        "/staticplugin"
    FILES
        "main.cpp"
)

qt_autogen_tools_initial_setup(moctestplugin)
