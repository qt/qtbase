# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_cmake_library(tst_qfactoryloader_staticplugin
    STATIC
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
)

qt_autogen_tools_initial_setup(tst_qfactoryloader_staticplugin)

target_compile_definitions(tst_qfactoryloader_staticplugin PRIVATE QT_STATICPLUGIN)
