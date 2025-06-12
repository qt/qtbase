# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(PostgreSQL MODULE)

#####################################################################
## QPSQLDriverPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QPSQLDriverPlugin
    OUTPUT_NAME qsqlpsql
    PLUGIN_TYPE sqldrivers
    SOURCES
        main.cpp
        qsql_psql.cpp qsql_psql_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
        QT_NO_CONTEXTLESS_CONNECT
    LIBRARIES
        PostgreSQL::PostgreSQL
        Qt::Core
        Qt::CorePrivate
        Qt::SqlPrivate
)

# PostgreSQL delivers header files that are not a part of PostgreSQL itself. When precompiled
# headers are processed, MinGW uses 'pthread.h' from the PostgreSQL installation directory.
# As result, we disable precompile headers for the plugin.
# See also QTBUG-90850.
if(MINGW)
    set_target_properties(QPSQLDriverPlugin
        PROPERTIES
            DISABLE_PRECOMPILE_HEADERS ON
    )
endif()
