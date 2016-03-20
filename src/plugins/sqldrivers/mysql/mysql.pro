TARGET = qsqlmysql

HEADERS += $$PWD/qsql_mysql_p.h
SOURCES += $$PWD/qsql_mysql.cpp $$PWD/main.cpp

QMAKE_CXXFLAGS *= $$QMAKE_CFLAGS_MYSQL
LIBS += $$QMAKE_LIBS_MYSQL

unix {
    isEmpty(QMAKE_LIBS_MYSQL) {
        !contains(LIBS, .*mysqlclient.*):!contains(LIBS, .*mysqld.*) {
            use_libmysqlclient_r:LIBS += -lmysqlclient_r
            else:LIBS += -lmysqlclient
        }
    }
} else {
    !contains(LIBS, .*mysql.*):!contains(LIBS, .*mysqld.*):LIBS += -llibmysql
}

OTHER_FILES += mysql.json

PLUGIN_CLASS_NAME = QMYSQLDriverPlugin
include(../qsqldriverbase.pri)
