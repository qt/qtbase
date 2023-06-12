# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsslserver Test:
#####################################################################

# Collect test data
list(APPEND test_data "certs")

qt_internal_add_test(tst_qsslserver
    SOURCES
        tst_qsslserver.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    TESTDATA ${test_data}
    BUNDLE_ANDROID_OPENSSL_LIBS
)
