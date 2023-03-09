# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QICOPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QICOPlugin
    OUTPUT_NAME qico
    PLUGIN_TYPE imageformats
    SOURCES
        main.cpp
        qicohandler.cpp qicohandler.h
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
    NO_UNITY_BUILD
)
