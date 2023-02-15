# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
