# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqlerror Test:
#####################################################################

qt_internal_add_test(tst_qsqlerror
    SOURCES
        tst_qsqlerror.cpp
    LIBRARIES
        Qt::Sql
)
