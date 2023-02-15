# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
    QT_TEST_SERVER_LIST "apache2"
)
