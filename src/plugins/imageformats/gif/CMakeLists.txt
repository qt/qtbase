# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QGifPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QGifPlugin
    OUTPUT_NAME qgif
    PLUGIN_TYPE imageformats
    SOURCES
        main.cpp
        qgifhandler.cpp qgifhandler_p.h
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)
