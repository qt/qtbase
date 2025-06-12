# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs

# input sqlite
set(INPUT_sqlite "undefined" CACHE STRING "")
set_property(CACHE INPUT_sqlite PROPERTY STRINGS undefined qt system)



#### Libraries

qt_find_package(DB2 MODULE PROVIDED_TARGETS DB2::DB2 MODULE_NAME sqldrivers QMAKE_LIB db2)
qt_find_package(MySQL MODULE PROVIDED_TARGETS MySQL::MySQL MODULE_NAME sqldrivers QMAKE_LIB mysql)
qt_find_package(PostgreSQL MODULE PROVIDED_TARGETS PostgreSQL::PostgreSQL MODULE_NAME sqldrivers QMAKE_LIB psql)
qt_find_package(Oracle MODULE PROVIDED_TARGETS Oracle::OCI MODULE_NAME sqldrivers QMAKE_LIB oci)
qt_find_package(ODBC PROVIDED_TARGETS ODBC::ODBC MODULE_NAME sqldrivers QMAKE_LIB odbc)
qt_find_package(SQLite3 PROVIDED_TARGETS SQLite::SQLite3 MODULE_NAME sqldrivers QMAKE_LIB sqlite3)
qt_find_package(Interbase MODULE
    PROVIDED_TARGETS Interbase::Interbase MODULE_NAME sqldrivers QMAKE_LIB ibase) # special case
qt_find_package(Mimer MODULE PROVIDED_TARGETS MimerSQL::MimerSQL MODULE_NAME sqldrivers QMAKE_LIB mimer)
if(NOT WIN32 AND QT_FEATURE_system_zlib)
    qt_add_qmake_lib_dependency(sqlite3 zlib)
endif()


#### Tests



#### Features

qt_feature("sql-db2" PRIVATE
    LABEL "DB2 (IBM)"
    CONDITION DB2_FOUND
)
qt_feature("sql-ibase" PRIVATE
    LABEL "InterBase"
    CONDITION Interbase_FOUND # special case
)
qt_feature("sql-mysql" PRIVATE
    LABEL "MySql"
    CONDITION MySQL_FOUND
)
qt_feature("sql-oci" PRIVATE
    LABEL "OCI (Oracle)"
    CONDITION Oracle_FOUND
)
qt_feature("sql-odbc" PRIVATE
    LABEL "ODBC"
    CONDITION QT_FEATURE_datestring AND ODBC_FOUND
)
qt_feature("sql-psql" PRIVATE
    LABEL "PostgreSQL"
    CONDITION PostgreSQL_FOUND
)
qt_feature("sql-sqlite" PRIVATE
    LABEL "SQLite"
    CONDITION QT_FEATURE_datestring
)
qt_feature("system-sqlite" PRIVATE SYSTEM_LIBRARY
    LABEL "  Using system provided SQLite"
    AUTODETECT OFF
    CONDITION QT_FEATURE_sql_sqlite AND SQLite3_FOUND
)
qt_feature("sql-mimer" PRIVATE
    LABEL "Mimer"
    CONDITION Mimer_FOUND
)

qt_configure_add_summary_section(NAME "Qt Sql Drivers")
qt_configure_add_summary_entry(ARGS "sql-db2")
qt_configure_add_summary_entry(ARGS "sql-ibase")
qt_configure_add_summary_entry(ARGS "sql-mysql")
qt_configure_add_summary_entry(ARGS "sql-oci")
qt_configure_add_summary_entry(ARGS "sql-odbc")
qt_configure_add_summary_entry(ARGS "sql-psql")
qt_configure_add_summary_entry(ARGS "sql-sqlite")
qt_configure_add_summary_entry(ARGS "system-sqlite")
qt_configure_add_summary_entry(ARGS "sql-mimer")
qt_configure_end_summary_section() # end of "Qt Sql Drivers" section
qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "Qt does not support compiling the Oracle database driver with MinGW, due to lack of such support from Oracle. Consider disabling the Oracle driver, as the current build will most likely fail."
    CONDITION WIN32 AND NOT MSVC AND QT_FEATURE_sql_oci
)
