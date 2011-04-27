HEADERS += $$PWD/qsql_db2.h
SOURCES += $$PWD/qsql_db2.cpp

unix {
    !contains(LIBS, .*db2.*):LIBS += -ldb2
} else:!win32-borland {
    !contains(LIBS, .*db2.*):LIBS += -ldb2cli
}
