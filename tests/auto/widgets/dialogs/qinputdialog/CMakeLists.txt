# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qinputdialog Test:
#####################################################################

qt_internal_add_test(tst_qinputdialog
    SOURCES
        tst_qinputdialog.cpp
    LIBRARIES
        Qt::Gui
        Qt::WidgetsPrivate
)
