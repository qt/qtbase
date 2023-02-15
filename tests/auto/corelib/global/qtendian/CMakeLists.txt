# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtendian Test:
#####################################################################

qt_internal_add_test(tst_qtendian
    SOURCES
        tst_qtendian.cpp
    LIBRARIES
        Qt::CorePrivate
)
