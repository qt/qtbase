# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtextodfwriter Test:
#####################################################################

qt_internal_add_test(tst_qtextodfwriter
    SOURCES
        tst_qtextodfwriter.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
