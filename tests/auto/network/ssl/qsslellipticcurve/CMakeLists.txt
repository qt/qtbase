# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsslellipticcurve Test:
#####################################################################

qt_internal_add_test(tst_qsslellipticcurve
    SOURCES
        tst_qsslellipticcurve.cpp
    LIBRARIES
        Qt::Network
    BUNDLE_ANDROID_OPENSSL_LIBS
)
