# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qrandomgenerator Test:
#####################################################################

qt_internal_add_test(tst_qrandomgenerator
    SOURCES
        tst_qrandomgenerator.cpp
    LIBRARIES
        Qt::CorePrivate
)
