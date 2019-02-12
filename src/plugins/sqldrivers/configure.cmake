

#### Inputs



#### Libraries

find_package(SQLite3)
set_package_properties(SQLite3 PROPERTIES TYPE OPTIONAL)


#### Tests



#### Features

qt_feature("sql_db2" PRIVATE
    LABEL "DB2 (IBM)"
    CONDITION libs.db2 OR FIXME
)
qt_feature("sql_ibase" PRIVATE
    LABEL "InterBase"
    CONDITION libs.ibase OR FIXME
)
qt_feature("sql_mysql" PRIVATE
    LABEL "MySql"
    CONDITION libs.mysql OR FIXME
)
qt_feature("sql_oci" PRIVATE
    LABEL "OCI (Oracle)"
    CONDITION libs.oci OR FIXME
)
qt_feature("sql_odbc" PRIVATE
    LABEL "ODBC"
    CONDITION QT_FEATURE_datestring AND libs.odbc OR FIXME
)
qt_feature("sql_psql" PRIVATE
    LABEL "PostgreSQL"
    CONDITION libs.psql OR FIXME
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
