# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from staticplugin.pro.

#####################################################################
## staticplugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(staticplugin
    STATIC
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
    MOC_OPTIONS
        "-M"
        "ExtraMetaData=StaticPlugin"
        "-M"
        "ExtraMetaData=foo"
)

#### Keys ignored in scope 1:.:.:staticplugin.pro:<TRUE>:
# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(staticplugin)

target_compile_definitions(staticplugin PRIVATE QT_STATICPLUGIN)
