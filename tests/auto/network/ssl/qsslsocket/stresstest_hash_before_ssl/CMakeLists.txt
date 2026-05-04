# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test_helper(opensslhash_before_ssl
    SOURCES
        opensslhash_before_ssl.cpp
    LIBRARIES
        Qt::Core
        Qt::Network
)

qt_internal_extend_target(opensslhash_before_ssl CONDITION QT_FEATURE_openssl_hash
    LIBRARIES
        WrapOpenSSL::WrapOpenSSL
)
