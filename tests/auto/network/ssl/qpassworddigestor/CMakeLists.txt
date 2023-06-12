# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpassworddigestor Test:
#####################################################################

qt_internal_add_test(tst_qpassworddigestor
    SOURCES
        tst_qpassworddigestor.cpp
    LIBRARIES
        Qt::Network
    BUNDLE_ANDROID_OPENSSL_LIBS
)
