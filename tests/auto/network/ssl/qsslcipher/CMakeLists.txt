# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsslcipher Test:
#####################################################################

qt_internal_add_test(tst_qsslcipher
    SOURCES
        tst_qsslcipher.cpp
    LIBRARIES
        Qt::Network
    BUNDLE_ANDROID_OPENSSL_LIBS
)
