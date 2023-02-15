# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_manual_qnetworkreply Binary:
#####################################################################

if (NOT QT_FEATURE_private_tests)
    return()
endif()

qt_internal_add_manual_test(tst_manual_qnetworkreply
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
        Qt::Test
)
