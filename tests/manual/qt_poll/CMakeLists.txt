# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qt_poll.pro.

#####################################################################
## tst_qt_poll Test:
#####################################################################

qt_internal_add_test(tst_qt_poll
    SOURCES
        ../../../src/corelib/kernel/qpoll.cpp
        tst_qt_poll.cpp
    INCLUDE_DIRECTORIES
        ../../../src/corelib/kernel
    LIBRARIES
        Qt::CorePrivate
        Qt::Network
)
