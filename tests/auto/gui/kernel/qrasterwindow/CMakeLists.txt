# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qrasterwindow Test:
#####################################################################

qt_internal_add_test(tst_qrasterwindow
    SOURCES
        tst_qrasterwindow.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)
