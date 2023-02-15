# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(staticplugin)

target_compile_definitions(staticplugin PRIVATE QT_STATICPLUGIN)
