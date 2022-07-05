# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from instancing.pro.

#####################################################################
## instancing Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_instancing
    GUI
    SOURCES
        instancing.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

# Resources:
set(instancing_resource_files
    "inst.frag.qsb"
    "inst.vert.qsb"
)

qt_internal_add_resource(tst_manual_instancing "instancing"
    PREFIX
        "/"
    FILES
        ${instancing_resource_files}
)


#### Keys ignored in scope 1:.:.:instancing.pro:<TRUE>:
# TEMPLATE = "app"
