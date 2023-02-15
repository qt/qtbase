# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qnetworkdatagram Test:
#####################################################################

qt_internal_add_test(tst_qnetworkdatagram
    SOURCES
        tst_qnetworkdatagram.cpp
    LIBRARIES
        Qt::Network
)
