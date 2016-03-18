TEMPLATE = subdirs

contains(sql-drivers, psql)     : SUBDIRS += psql
contains(sql-drivers, mysql)    : SUBDIRS += mysql
contains(sql-drivers, odbc)     : SUBDIRS += odbc
contains(sql-drivers, tds)      : SUBDIRS += tds
contains(sql-drivers, oci)      : SUBDIRS += oci
contains(sql-drivers, db2)      : SUBDIRS += db2
contains(sql-drivers, sqlite)   : SUBDIRS += sqlite
contains(sql-drivers, sqlite2)  : SUBDIRS += sqlite2
contains(sql-drivers, ibase)    : SUBDIRS += ibase
