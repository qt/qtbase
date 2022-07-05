# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qsocketnotifier.pro.

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

#### Keys ignored in scope 1:.:.:qsocketnotifier.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qsocketnotifier CONDITION WIN32
    LIBRARIES
        ws2_32
)
