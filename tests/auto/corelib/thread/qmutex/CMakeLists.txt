# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmutex Test:
#####################################################################

qt_internal_add_test(tst_qmutex
    SOURCES
        tst_qmutex.cpp
    LIBRARIES
        Qt::CorePrivate
)
