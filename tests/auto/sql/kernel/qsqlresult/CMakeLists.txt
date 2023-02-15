# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqlresult Test:
#####################################################################

qt_internal_add_test(tst_qsqlresult
    SOURCES
        testsqldriver.h
        tst_qsqlresult.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
)
