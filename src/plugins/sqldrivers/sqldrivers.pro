TEMPLATE = subdirs

sqldrivers_standalone {
    _QMAKE_CACHE_ = $$shadowed($$SQLDRV_SRC_TREE)/.qmake.conf
    load(qt_configure)
}

qtConfig(sql-psql)     : SUBDIRS += psql
qtConfig(sql-mysql)    : SUBDIRS += mysql
qtConfig(sql-odbc)     : SUBDIRS += odbc
qtConfig(sql-tds)      : SUBDIRS += tds
qtConfig(sql-oci)      : SUBDIRS += oci
qtConfig(sql-db2)      : SUBDIRS += db2
qtConfig(sql-sqlite)   : SUBDIRS += sqlite
qtConfig(sql-sqlite2)  : SUBDIRS += sqlite2
qtConfig(sql-ibase)    : SUBDIRS += ibase
