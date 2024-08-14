# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(tst_stenciloutline
    GUI
    SOURCES
        stenciloutline.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

qt_internal_add_resource(tst_stenciloutline "stenciloutline"
    PREFIX
        "/"
    FILES
        "material.frag.qsb"
        "material.vert.qsb"
)
