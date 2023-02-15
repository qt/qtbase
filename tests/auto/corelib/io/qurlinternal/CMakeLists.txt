# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qurlinternal Test:
#####################################################################

qt_internal_add_test(tst_qurlinternal
    SOURCES
        tst_qurlinternal.cpp
        utf8data.cpp
    LIBRARIES
        Qt::CorePrivate
)
