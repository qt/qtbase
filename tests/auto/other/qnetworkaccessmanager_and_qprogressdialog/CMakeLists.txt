# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
