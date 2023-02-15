# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpromise Test:
#####################################################################

qt_internal_add_test(tst_qpromise
    SOURCES
        tst_qpromise.cpp
    LIBRARIES
        Qt::CorePrivate
)
