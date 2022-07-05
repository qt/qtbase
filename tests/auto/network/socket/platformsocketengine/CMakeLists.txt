# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from platformsocketengine.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_platformsocketengine Test:
#####################################################################

qt_internal_add_test(tst_platformsocketengine
    SOURCES
        tst_platformsocketengine.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    QT_TEST_SERVER_LIST "cyrus" # special case
)

#### Keys ignored in scope 1:.:.:platformsocketengine.pro:<TRUE>:
# MOC_DIR = "tmp"
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_platformsocketengine CONDITION WIN32
    LIBRARIES
        ws2_32
)
