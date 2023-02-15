# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtconcurrentiteratekernel Test:
#####################################################################

qt_internal_add_test(tst_qtconcurrentiteratekernel
    SOURCES
        tst_qtconcurrentiteratekernel.cpp
    LIBRARIES
        Qt::Concurrent
)
