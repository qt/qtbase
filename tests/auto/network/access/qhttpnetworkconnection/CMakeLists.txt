# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qhttpnetworkconnection.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qhttpnetworkconnection Test:
#####################################################################

qt_internal_add_test(tst_qhttpnetworkconnection
    SOURCES
        tst_qhttpnetworkconnection.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "apache2" # special case
)

#### Keys ignored in scope 1:.:.:qhttpnetworkconnection.pro:<TRUE>:
# QT_TEST_SERVER_LIST = "apache2"
# _REQUIREMENTS = "qtConfig(private_tests)"
