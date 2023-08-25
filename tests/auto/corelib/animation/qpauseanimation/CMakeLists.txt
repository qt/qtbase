# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpauseanimation Test:
#####################################################################

qt_internal_add_test(tst_qpauseanimation
    SOURCES
        tst_qpauseanimation.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
)
