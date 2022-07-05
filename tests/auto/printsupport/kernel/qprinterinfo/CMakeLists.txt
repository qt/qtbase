# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qprinterinfo.pro.

#####################################################################
## tst_qprinterinfo Test:
#####################################################################

qt_internal_add_test(tst_qprinterinfo
    SOURCES
        tst_qprinterinfo.cpp
    DEFINES
        QT_USE_USING_NAMESPACE
    LIBRARIES
        Qt::Gui
        Qt::PrintSupport
)
