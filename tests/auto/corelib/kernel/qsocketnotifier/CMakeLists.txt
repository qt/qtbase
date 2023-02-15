# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qsocketnotifier Test:
#####################################################################

qt_internal_add_test(tst_qsocketnotifier
    SOURCES
        tst_qsocketnotifier.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
        Qt::NetworkPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsocketnotifier CONDITION WIN32
    LIBRARIES
        ws2_32
)
