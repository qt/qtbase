# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from platforminputcontexts.pro.

if(QT_FEATURE_xkbcommon)
    add_subdirectory(compose)
endif()
if(QT_FEATURE_xkbcommon AND TARGET Qt::DBus AND UNIX AND NOT MACOS)
    add_subdirectory(ibus)
endif()
