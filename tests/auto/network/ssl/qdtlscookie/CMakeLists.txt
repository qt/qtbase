# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdtlscookie Test:
#####################################################################

qt_internal_add_test(tst_qdtlscookie
    SOURCES
        tst_qdtlscookie.cpp
    LIBRARIES
        Qt::NetworkPrivate
    BUNDLE_ANDROID_OPENSSL_LIBS
)
