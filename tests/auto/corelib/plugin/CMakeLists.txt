# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from plugin.pro.

if(QT_BUILD_SHARED_LIBS)
    add_subdirectory(qfactoryloader)
endif()
add_subdirectory(quuid)
if(QT_FEATURE_library)
    add_subdirectory(qpluginloader)
    add_subdirectory(qlibrary)
endif()
if(QT_BUILD_SHARED_LIBS AND QT_FEATURE_library)
    add_subdirectory(qplugin)
endif()
