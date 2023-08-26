# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qnetworkinterface Test:
#####################################################################

qt_internal_add_test(tst_qnetworkinterface
    SOURCES
        tst_qnetworkinterface.cpp
    LIBRARIES
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "apache2"
)
