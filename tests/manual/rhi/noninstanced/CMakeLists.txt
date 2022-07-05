# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from noninstanced.pro.

#####################################################################
## noninstanced Binary:
#####################################################################

qt_internal_add_manual_test(noninstanced
    GUI
    SOURCES
        noninstanced.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(noninstanced_resource_files
    "material.frag.qsb"
    "material.vert.qsb"
)

qt_internal_add_resource(noninstanced "noninstanced"
    PREFIX
        "/"
    FILES
        ${noninstanced_resource_files}
)


#### Keys ignored in scope 1:.:.:noninstanced.pro:<TRUE>:
# TEMPLATE = "app"
