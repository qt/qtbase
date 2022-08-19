# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(polygonmode
    GUI
    SOURCES
        polygonmode.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(polygonmode_resource_files
    "test.vert.qsb"
    "test.frag.qsb"
)

qt_internal_add_resource(polygonmode "polygonmode"
    PREFIX
        "/"
    FILES
        ${polygonmode_resource_files}
)
