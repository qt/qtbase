# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()

qt_internal_add_test(tst_qhttpheaderparser
    SOURCES
        tst_qhttpheaderparser.cpp
    LIBRARIES
        Qt::NetworkPrivate
)
