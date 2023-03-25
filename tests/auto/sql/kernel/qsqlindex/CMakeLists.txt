# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqlindex Test:
#####################################################################

qt_internal_add_test(tst_qsqlindex
    SOURCES
        tst_qsqlindex.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
)
