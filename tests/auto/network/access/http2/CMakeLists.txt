# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
    BUNDLE_ANDROID_OPENSSL_LIBS
)
