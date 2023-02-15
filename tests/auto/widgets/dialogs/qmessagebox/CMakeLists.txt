# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmessagebox Test:
#####################################################################

qt_internal_add_test(tst_qmessagebox
    SOURCES
        tst_qmessagebox.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
