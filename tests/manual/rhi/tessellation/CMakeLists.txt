# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(tessellation
    GUI
    SOURCES
        tessellation.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(tessellation_resource_files
    "test.vert.qsb"
    "test.tesc.qsb"
    "test.tese.qsb"
    "test.frag.qsb"
)

qt_internal_add_resource(tessellation "tessellation"
    PREFIX
        "/"
    FILES
        ${tessellation_resource_files}
)
