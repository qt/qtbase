# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsslkey Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    keys/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    rsa-*.pem)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qsslkey
    SOURCES
        tst_qsslkey.cpp
    LIBRARIES
        Qt::Network
    TESTDATA ${test_data}
    BUNDLE_ANDROID_OPENSSL_LIBS
)

qt_internal_extend_target(tst_qsslkey CONDITION QT_FEATURE_developer_build AND QT_FEATURE_openssl_linked
    LIBRARIES
        WrapOpenSSL::WrapOpenSSL
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsslkey CONDITION QT_FEATURE_developer_build
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
)
