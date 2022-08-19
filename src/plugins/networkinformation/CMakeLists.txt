# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(WIN32 AND QT_FEATURE_networklistmanager)
    add_subdirectory(networklistmanager)
endif()

if(LINUX AND TARGET Qt::DBus)
    add_subdirectory(networkmanager)
endif()

if(APPLE)
    add_subdirectory(scnetworkreachability)
endif()

if(ANDROID)
    add_subdirectory(android)
endif()

if(QT_FEATURE_glib AND TARGET GLIB2::GOBJECT AND TARGET GLIB2::GIO)
    add_subdirectory(glib)
endif()
