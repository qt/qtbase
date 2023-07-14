# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QODBCDriverPlugin Plugin:
#####################################################################

qt_find_package(ODBC)

qt_internal_add_plugin(QODBCDriverPlugin
    OUTPUT_NAME qsqlodbc
    PLUGIN_TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_odbc.cpp qsql_odbc_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_CONTEXTLESS_CONNECT
    LIBRARIES
        ODBC::ODBC
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(QODBCDriverPlugin CONDITION UNIX
    DEFINES
        UNICODE
)
