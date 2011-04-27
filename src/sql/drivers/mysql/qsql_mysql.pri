HEADERS += $$PWD/qsql_mysql.h
SOURCES += $$PWD/qsql_mysql.cpp

unix {
    isEmpty(QT_LFLAGS_MYSQL) {
        !contains(LIBS, .*mysqlclient.*):!contains(LIBS, .*mysqld.*) {
            use_libmysqlclient_r:LIBS += -lmysqlclient_r
            else:LIBS += -lmysqlclient
        }
    } else {
        LIBS *= $$QT_LFLAGS_MYSQL
        QMAKE_CXXFLAGS *= $$QT_CFLAGS_MYSQL
    }
} else {
    !contains(LIBS, .*mysql.*):!contains(LIBS, .*mysqld.*):LIBS += -llibmysql
}
