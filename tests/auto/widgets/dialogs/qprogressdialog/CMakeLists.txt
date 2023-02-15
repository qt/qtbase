# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qprogressdialog Test:
#####################################################################

qt_internal_add_test(tst_qprogressdialog
    SOURCES
        tst_qprogressdialog.cpp
    LIBRARIES
        Qt::Gui
        Qt::Widgets
)
