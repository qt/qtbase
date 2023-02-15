# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfuturewatcher Test:
#####################################################################

qt_internal_add_test(tst_qfuturewatcher
    SOURCES
        tst_qfuturewatcher.cpp
    LIBRARIES
        Qt::Concurrent
        Qt::CorePrivate
)
