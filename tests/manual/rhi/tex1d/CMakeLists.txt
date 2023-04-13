# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(tex1d
    GUI
    SOURCES
        tex1d.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

set(tex1d_resource_files
    "texture1d.vert.qsb"
    "texture1d.frag.qsb"
)

qt_internal_add_resource(tex1d "tex1d"
    PREFIX
        "/"
    FILES
        ${tex1d_resource_files}
)
