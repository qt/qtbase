# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqldatabase Test:
#####################################################################

qt_internal_add_test(tst_qsqldatabase
    SOURCES
        tst_qsqldatabase.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
)
