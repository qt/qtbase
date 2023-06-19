# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(multiview
    GUI
    SOURCES
        multiview.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

qt_internal_add_resource(multiview "multiview"
    PREFIX
        "/"
    FILES
        "multiview.vert.qsb"
        "multiview.frag.qsb"
        "texture.vert.qsb"
        "texture.frag.qsb"
)
