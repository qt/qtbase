# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from gif.pro.

#####################################################################
## QGifPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QGifPlugin
    OUTPUT_NAME qgif
    PLUGIN_TYPE imageformats
    SOURCES
        main.cpp
        qgifhandler.cpp qgifhandler_p.h
    LIBRARIES # special case
        Qt::Gui # special case
        Qt::GuiPrivate # special case
)

#### Keys ignored in scope 1:.:.:gif.pro:<TRUE>:
# OTHER_FILES = "gif.json"
