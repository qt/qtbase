# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qssldiffiehellmanparameters Test:
#####################################################################

qt_internal_add_test(tst_qssldiffiehellmanparameters
    SOURCES
        tst_qssldiffiehellmanparameters.cpp
    LIBRARIES
        Qt::Network
    BUNDLE_ANDROID_OPENSSL_LIBS
)
