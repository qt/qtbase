# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmakearray Test:
#####################################################################

qt_internal_add_test(tst_qmakearray
    SOURCES
        tst_qmakearray.cpp
    LIBRARIES
        Qt::CorePrivate
)
