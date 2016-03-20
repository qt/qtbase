TARGET = qsqlodbc

HEADERS += $$PWD/qsql_odbc_p.h
SOURCES += $$PWD/qsql_odbc.cpp $$PWD/main.cpp

unix {
    DEFINES += UNICODE
    !contains(LIBS, .*odbc.*) {
        osx:LIBS += -liodbc
        else:LIBS += $$QMAKE_LIBS_ODBC
    }
} else {
    LIBS *= -lodbc32
}

OTHER_FILES += odbc.json

PLUGIN_CLASS_NAME = QODBCDriverPlugin
include(../qsqldriverbase.pri)
