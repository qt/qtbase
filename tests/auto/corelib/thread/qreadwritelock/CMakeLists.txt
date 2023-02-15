# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qreadwritelock Test:
#####################################################################

qt_internal_add_test(tst_qreadwritelock
    SOURCES
        tst_qreadwritelock.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
)
