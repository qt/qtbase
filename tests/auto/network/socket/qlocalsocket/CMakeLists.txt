# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(socketprocess)
qt_internal_add_test(tst_qlocalsocket
    SOURCES
        tst_qlocalsocket.cpp
    DEFINES
        QLOCALSERVER_DEBUG
        QLOCALSOCKET_DEBUG
    LIBRARIES
        Qt::Network
        Qt::TestPrivate
)
add_dependencies(tst_qlocalsocket socketprocess)
