# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qobjectrace Test:
#####################################################################

qt_internal_add_test(tst_qobjectrace
    SOURCES
        tst_qobjectrace.cpp
    LIBRARIES
        Qt::TestPrivate
)
