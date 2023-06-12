# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdtls Test:
#####################################################################

# Collect test data
list(APPEND test_data "certs")

qt_internal_add_test(tst_qdtls
    SOURCES
        tst_qdtls.cpp
    LIBRARIES
        Qt::NetworkPrivate
    TESTDATA ${test_data}
    BUNDLE_ANDROID_OPENSSL_LIBS
)
