# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtextformat Test:
#####################################################################

qt_internal_add_test(tst_qtextformat
    SOURCES
        tst_qtextformat.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
