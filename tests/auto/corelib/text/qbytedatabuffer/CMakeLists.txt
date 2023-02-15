# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qbytedatabuffer Test:
#####################################################################

qt_internal_add_test(tst_qbytedatabuffer
    SOURCES
        tst_qbytedatabuffer.cpp
    LIBRARIES
        Qt::CorePrivate
)
