# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtconcurrentmedian Test:
#####################################################################

qt_internal_add_test(tst_qtconcurrentmedian
    SOURCES
        tst_qtconcurrentmedian.cpp
    LIBRARIES
        Qt::Concurrent
)
