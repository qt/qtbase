# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(GTK3)

if(QT_FEATURE_xlib)
    qt_find_package(X11)
endif()

#####################################################################
## QGtk3ThemePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QGtk3ThemePlugin
    OUTPUT_NAME qgtk3
    PLUGIN_TYPE platformthemes
    DEFAULT_IF FALSE
    SOURCES
        main.cpp
        qgtk3dialoghelpers.cpp qgtk3dialoghelpers.h
        qgtk3menu.cpp qgtk3menu.h
        qgtk3theme.cpp qgtk3theme.h
        qgtk3interface.cpp qgtk3interface_p.h
        qgtk3storage.cpp qgtk3storage_p.h
        qgtk3json.cpp qgtk3json_p.h
    DEFINES
        GDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_6
    LIBRARIES
        PkgConfig::GTK3
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

qt_internal_extend_target(QGtk3ThemePlugin CONDITION QT_FEATURE_xlib
    LIBRARIES
        X11::X11
)
