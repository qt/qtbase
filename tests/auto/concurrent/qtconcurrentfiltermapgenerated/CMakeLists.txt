# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtconcurrentfiltermapgenerated Test:
#####################################################################

qt_internal_add_test(tst_qtconcurrentfiltermapgenerated
    SOURCES
        tst_qtconcurrent_selected_tests.cpp
        tst_qtconcurrentfiltermapgenerated.cpp tst_qtconcurrentfiltermapgenerated.h
    LIBRARIES
        Qt::Concurrent
)
