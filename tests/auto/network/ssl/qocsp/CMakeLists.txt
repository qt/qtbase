# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qocsp Test:
#####################################################################

qt_internal_add_test(tst_qocsp
    SOURCES
        tst_qocsp.cpp
    LIBRARIES
        Qt::Network
        Qt::NetworkPrivate
    BUNDLE_ANDROID_OPENSSL_LIBS
)

qt_internal_extend_target(tst_qocsp CONDITION QT_FEATURE_openssl_linked
    LIBRARIES
        WrapOpenSSL::WrapOpenSSL
)
