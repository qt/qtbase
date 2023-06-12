# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsslerror Test:
#####################################################################

qt_internal_add_test(tst_qsslerror
    SOURCES
        tst_qsslerror.cpp
    LIBRARIES
        Qt::Network
    BUNDLE_ANDROID_OPENSSL_LIBS
)
