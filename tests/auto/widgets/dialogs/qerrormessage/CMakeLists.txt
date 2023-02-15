# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qerrormessage Test:
#####################################################################

qt_internal_add_test(tst_qerrormessage
    SOURCES
        tst_qerrormessage.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
