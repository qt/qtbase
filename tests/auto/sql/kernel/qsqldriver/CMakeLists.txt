# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqldriver Test:
#####################################################################

qt_internal_add_test(tst_qsqldriver
    SOURCES
        tst_qsqldriver.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
)
