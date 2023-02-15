# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtextlayout Test:
#####################################################################

qt_internal_add_test(tst_qtextlayout
    SOURCES
        tst_qtextlayout.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
