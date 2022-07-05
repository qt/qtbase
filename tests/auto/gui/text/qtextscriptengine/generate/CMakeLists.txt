# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from generate.pro.

#####################################################################
## generate Binary:
#####################################################################

qt_internal_add_executable(generate
    GUI
    SOURCES
        main.cpp
    INCLUDE_DIRECTORIES
        /usr/include/freetype2
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
)
