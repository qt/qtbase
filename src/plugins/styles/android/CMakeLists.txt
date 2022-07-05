# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from android.pro.

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

#### Keys ignored in scope 1:.:.:android.pro:<TRUE>:
# DISTFILES = "androidstyle.json"
