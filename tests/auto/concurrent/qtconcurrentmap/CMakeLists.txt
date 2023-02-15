# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtconcurrentmap Test:
#####################################################################

qt_internal_add_test(tst_qtconcurrentmap
    SOURCES
        tst_qtconcurrentmap.cpp
    LIBRARIES
        Qt::Concurrent
)

## Scopes:
#####################################################################
