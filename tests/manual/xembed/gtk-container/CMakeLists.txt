# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(GTK3 MODULE)
qt_find_package(X11 MODULE)

#####################################################################
## gtk-container Binary:
#####################################################################

qt_internal_add_manual_test(gtk-container
    GUI
    SOURCES
        gtk-container.cpp
        DEFINES
        GDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_6
    LIBRARIES
        PkgConfig::GTK3
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        X11::X11
)
