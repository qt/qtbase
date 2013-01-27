HEADERS += $$PWD/qsql_sqlite.h
SOURCES += $$PWD/qsql_sqlite.cpp

!system-sqlite:!contains(LIBS, .*sqlite3.*) {
    include($$PWD/../../../3rdparty/sqlite.pri)
} else {
    LIBS += $$QT_LFLAGS_SQLITE
    QMAKE_CXXFLAGS *= $$QT_CFLAGS_SQLITE
}
