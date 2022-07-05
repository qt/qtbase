# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qsqlresult.pro.

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
