TEMPLATE = subdirs
QT_FOR_CONFIG += sql

qtConfig(sql-psql)     : SUBDIRS += psql
qtConfig(sql-mysql)    : SUBDIRS += mysql
qtConfig(sql-odbc)     : SUBDIRS += odbc
qtConfig(sql-tds)      : SUBDIRS += tds
qtConfig(sql-oci)      : SUBDIRS += oci
qtConfig(sql-db2)      : SUBDIRS += db2
qtConfig(sql-sqlite)   : SUBDIRS += sqlite
qtConfig(sql-sqlite2)  : SUBDIRS += sqlite2
qtConfig(sql-ibase)    : SUBDIRS += ibase
