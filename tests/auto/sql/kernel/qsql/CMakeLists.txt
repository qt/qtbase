# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsql Test:
#####################################################################

qt_internal_add_test(tst_qsql
    SOURCES
        tst_qsql.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SqlPrivate
)
