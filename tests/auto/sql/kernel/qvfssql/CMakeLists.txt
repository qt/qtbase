# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsqlfield Test:
#####################################################################

qt_internal_add_test(tst_qvfssql
    SOURCES
        tst_qvfssql.cpp
    LIBRARIES
        Qt::SqlPrivate
)

set(qvfssql_resource_files
    "sample.db"
)

qt_internal_add_resource(tst_qvfssql "tst_qvfssql"
    PREFIX
        "/ro/"
    FILES
        ${qvfssql_resource_files}
)
