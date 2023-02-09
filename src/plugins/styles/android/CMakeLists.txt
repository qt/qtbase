# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QAndroidStylePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QAndroidStylePlugin
    OUTPUT_NAME qandroidstyle
    PLUGIN_TYPE styles
    SOURCES
        main.cpp
        qandroidstyle.cpp qandroidstyle_p.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::WidgetsPrivate
)
