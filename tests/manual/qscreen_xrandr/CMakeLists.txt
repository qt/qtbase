# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qscreen2 Test:
#####################################################################

qt_internal_add_test(tst_qscreen_xrandr
    SOURCES
        tst_qscreen_xrandr.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
