# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtextcursor Test:
#####################################################################

qt_internal_add_test(tst_qtextcursor
    SOURCES
        tst_qtextcursor.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
