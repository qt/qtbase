# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qasn1element Test:
#####################################################################

qt_internal_add_test(tst_qasn1element
    SOURCES
        tst_qasn1element.cpp
    LIBRARIES
        Qt::Network
        Qt::NetworkPrivate
    BUNDLE_ANDROID_OPENSSL_LIBS
)
