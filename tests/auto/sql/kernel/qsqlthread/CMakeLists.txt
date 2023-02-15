# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqlthread Test:
#####################################################################

qt_internal_add_test(tst_qsqlthread
    SOURCES
        tst_qsqlthread.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
)
