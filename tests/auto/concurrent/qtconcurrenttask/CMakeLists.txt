# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtconcurrenttask Test:
#####################################################################

qt_internal_add_test(tst_qtconcurrenttask
    SOURCES
        tst_qtconcurrenttask.cpp
    LIBRARIES
        Qt::Concurrent
)
