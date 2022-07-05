# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qpluginloader.pro.

add_subdirectory(lib)
add_subdirectory(staticplugin)
add_subdirectory(theplugin)
add_subdirectory(tst)
if(UNIX AND NOT ANDROID AND NOT APPLE)
    add_subdirectory(almostplugin)
endif()
if(MACOS AND QT_FEATURE_private_tests AND TARGET Qt::Gui)
    add_subdirectory(machtest)
endif()
