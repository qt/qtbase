

#### Inputs



#### Libraries

qt_find_package(DB2 PROVIDED_TARGETS DB2::DB2)
qt_find_package(MySQL PROVIDED_TARGETS MySQL::MySQL)
qt_find_package(PostgreSQL PROVIDED_TARGETS PostgreSQL::PostgreSQL)
qt_find_package(Oracle PROVIDED_TARGETS Oracle::OCI)
qt_find_package(ODBC PROVIDED_TARGETS ODBC::ODBC)
qt_find_package(SQLite3 PROVIDED_TARGETS SQLite::SQLite3)


#### Tests



#### Features

qt_feature("sql-db2" PRIVATE
    LABEL "DB2 (IBM)"
    CONDITION DB2_FOUND
)
qt_feature("sql-ibase" PRIVATE
    LABEL "InterBase"
    CONDITION libs.ibase OR FIXME
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
    CONDITION QT_FEATURE_datestring AND SQLite3_FOUND
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
qt_configure_end_summary_section() # end of "Qt Sql Drivers" section
qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "Qt does not support compiling the Oracle database driver with MinGW, due to lack of such support from Oracle. Consider disabling the Oracle driver, as the current build will most likely fail."
    CONDITION WIN32 AND NOT MSVC AND QT_FEATURE_sql_oci
)
