# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qnetconmonitor Binary:
#####################################################################

if (NOT QT_FEATURE_private_tests)
    return()
endif()

qt_internal_add_manual_test(qnetconmonitor
    SOURCES
        tst_qnetconmonitor.cpp
    LIBRARIES
        Qt::NetworkPrivate
        Qt::Test
)
