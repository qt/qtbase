# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtconcurrentfilter Test:
#####################################################################

qt_internal_add_test(tst_qtconcurrentfilter
    SOURCES
        tst_qtconcurrentfilter.cpp
    LIBRARIES
        Qt::Concurrent
)
