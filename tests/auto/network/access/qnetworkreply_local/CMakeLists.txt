# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
qt_internal_add_test(tst_qnetworkreply_local
    SOURCES
        minihttpserver.h
        tst_qnetworkreply_local.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    BUNDLE_ANDROID_OPENSSL_LIBS
)
