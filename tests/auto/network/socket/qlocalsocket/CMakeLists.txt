# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qlocalsocket.pro.

# add_subdirectory(test) # special case remove
add_subdirectory(socketprocess)
qt_internal_add_test(tst_qlocalsocket
    SOURCES
        tst_qlocalsocket.cpp
    DEFINES
        QLOCALSERVER_DEBUG
        QLOCALSOCKET_DEBUG
    LIBRARIES
        Qt::Network
)
add_dependencies(tst_qlocalsocket socketprocess)
# special case end
