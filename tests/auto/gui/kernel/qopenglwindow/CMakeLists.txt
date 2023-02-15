# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qopenglwindow Test:
#####################################################################

qt_internal_add_test(tst_qopenglwindow
    SOURCES
        tst_qopenglwindow.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
)
