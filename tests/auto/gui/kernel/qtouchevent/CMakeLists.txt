# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtouchevent Test:
#####################################################################

qt_internal_add_test(tst_qtouchevent
    SOURCES
        tst_qtouchevent.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
