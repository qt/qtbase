# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqlrecord Test:
#####################################################################

qt_internal_add_test(tst_qsqlrecord
    SOURCES
        tst_qsqlrecord.cpp
    LIBRARIES
        Qt::Sql
)
