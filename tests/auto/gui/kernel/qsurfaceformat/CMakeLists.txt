# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsurfaceformat Test:
#####################################################################

qt_internal_add_test(tst_qsurfaceformat
    SOURCES
        tst_qsurfaceformat.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
