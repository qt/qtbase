# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qhttpnetworkreply Test:
#####################################################################

qt_internal_add_test(tst_qhttpnetworkreply
    SOURCES
        tst_qhttpnetworkreply.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
)
