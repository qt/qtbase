# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtconcurrentrun Test:
#####################################################################

qt_internal_add_test(tst_qtconcurrentrun
    SOURCES
        tst_qtconcurrentrun.cpp
    LIBRARIES
        Qt::Concurrent
        Qt::TestPrivate
)

## Scopes:
#####################################################################
