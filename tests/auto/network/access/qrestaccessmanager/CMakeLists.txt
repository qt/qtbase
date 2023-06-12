# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qrestaccessmanager
    SOURCES
        tst_qrestaccessmanager.cpp
        httptestserver.cpp httptestserver_p.h
    LIBRARIES
        Qt::Network
        Qt::CorePrivate
)
