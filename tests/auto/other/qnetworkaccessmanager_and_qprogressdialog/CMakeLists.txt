# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qnetworkaccessmanager_and_qprogressdialog.pro.

#####################################################################
## tst_qnetworkaccessmanager_and_qprogressdialog Test:
#####################################################################

qt_internal_add_test(tst_qnetworkaccessmanager_and_qprogressdialog
    SOURCES
        tst_qnetworkaccessmanager_and_qprogressdialog.cpp
    LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Widgets
    QT_TEST_SERVER_LIST "apache2"
)
