# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QMYSQLDriverPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QMYSQLDriverPlugin
    OUTPUT_NAME qsqlmysql
    PLUGIN_TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_mysql.cpp qsql_mysql_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_CONTEXTLESS_CONNECT
    LIBRARIES
        MySQL::MySQL
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
)

qt_internal_force_macos_intel_arch(QMYSQLDriverPlugin)
