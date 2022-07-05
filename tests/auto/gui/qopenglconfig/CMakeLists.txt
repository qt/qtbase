# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qopenglconfig.pro.

#####################################################################
## tst_qopenglconfig Test:
#####################################################################

# Collect test data
list(APPEND test_data "buglist.json")

qt_internal_add_test(tst_qopenglconfig
    SOURCES
        tst_qopenglconfig.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)
