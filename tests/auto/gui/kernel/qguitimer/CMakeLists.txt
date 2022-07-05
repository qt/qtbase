# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qguitimer.pro.

#####################################################################
## tst_qguitimer Test:
#####################################################################

qt_internal_add_test(tst_qguitimer
    SOURCES
        ../../../corelib/kernel/qtimer/tst_qtimer.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
)
