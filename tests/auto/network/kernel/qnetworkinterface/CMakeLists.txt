# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qnetworkinterface.pro.

#####################################################################
## tst_qnetworkinterface Test:
#####################################################################

qt_internal_add_test(tst_qnetworkinterface
    SOURCES
        tst_qnetworkinterface.cpp
    LIBRARIES
        Qt::Network
    QT_TEST_SERVER_LIST "apache2"
)
