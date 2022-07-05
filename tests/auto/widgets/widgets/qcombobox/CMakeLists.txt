# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qcombobox.pro.

#####################################################################
## tst_qcombobox Test:
#####################################################################

# Collect test data
list(APPEND test_data "qtlogo.png")
list(APPEND test_data "qtlogoinverted.png")

qt_internal_add_test(tst_qcombobox
    SOURCES
        tst_qcombobox.cpp
    DEFINES
        QTEST_QPA_MOUSE_HANDLING
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
    TESTDATA ${test_data}
)
