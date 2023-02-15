# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqltablemodel Test:
#####################################################################

qt_internal_add_test(tst_qsqltablemodel
    SOURCES
        tst_qsqltablemodel.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Sql
        Qt::SqlPrivate
)
