# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from http2.pro.

#####################################################################
## tst_http2 Test:
#####################################################################

qt_internal_add_test(tst_http2
    SOURCES
        http2srv.cpp http2srv.h
        tst_http2.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
        Qt::TestPrivate
)
