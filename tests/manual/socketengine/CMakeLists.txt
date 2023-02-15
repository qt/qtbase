# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_socketengine Binary:
#####################################################################

if (NOT QT_FEATURE_private_tests)
    return()
endif()

qt_internal_add_manual_test(tst_socketengine
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
        Qt::Test
)
