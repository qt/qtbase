# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qget Binary:
#####################################################################

qt_internal_add_manual_test(qget
    SOURCES
        downloadmanager.cpp
        qget.cpp qget.h
        transferitem.cpp
    LIBRARIES
        Qt::Network
)
