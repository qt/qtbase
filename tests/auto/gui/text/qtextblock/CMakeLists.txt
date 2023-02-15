# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtextblock Test:
#####################################################################

qt_internal_add_test(tst_qtextblock
    SOURCES
        tst_qtextblock.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
