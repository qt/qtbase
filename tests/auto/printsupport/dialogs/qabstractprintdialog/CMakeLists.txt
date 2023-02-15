# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qabstractprintdialog Test:
#####################################################################

qt_internal_add_test(tst_qabstractprintdialog
    SOURCES
        tst_qabstractprintdialog.cpp
    LIBRARIES
        Qt::Gui
        Qt::PrintSupport
        Qt::Widgets
)
