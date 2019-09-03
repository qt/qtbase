

#### Inputs



#### Libraries

qt_find_package(DB2 PROVIDED_TARGETS DB2::DB2)
qt_find_package(MySQL PROVIDED_TARGETS MySQL::MySQL)
qt_find_package(PostgreSQL PROVIDED_TARGETS PostgreSQL::PostgreSQL)
qt_find_package(ODBC PROVIDED_TARGETS ODBC::ODBC)
qt_find_package(SQLite3 PROVIDED_TARGETS SQLite::SQLite3)


#### Tests



#### Features

qt_feature("sql_db2" PRIVATE
    LABEL "DB2 (IBM)"
    CONDITION DB2_FOUND
)
qt_feature("sql_ibase" PRIVATE
    LABEL "InterBase"
    CONDITION libs.ibase OR FIXME
)
qt_feature("sql_mysql" PRIVATE
    LABEL "MySql"
    CONDITION MySQL_FOUND
)
qt_feature("sql_oci" PRIVATE
    LABEL "OCI (Oracle)"
    CONDITION libs.oci OR FIXME
)
qt_feature("sql_odbc" PRIVATE
    LABEL "ODBC"
    CONDITION QT_FEATURE_datestring AND ODBC_FOUND
)
qt_feature("sql_psql" PRIVATE
    LABEL "PostgreSQL"
    CONDITION PostgreSQL_FOUND
)
qt_feature("sql_sqlite2" PRIVATE
    LABEL "SQLite2"
    CONDITION libs.sqlite2 OR FIXME
)
qt_feature("sql_sqlite" PRIVATE
    LABEL "SQLite"
    CONDITION QT_FEATURE_datestring AND SQLite3_FOUND
)
qt_feature("sql_tds" PRIVATE
    LABEL "TDS (Sybase)"
    CONDITION QT_FEATURE_datestring AND libs.tds OR FIXME
)
