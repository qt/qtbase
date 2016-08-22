TARGET = qsqlibase

HEADERS += $$PWD/qsql_ibase_p.h
SOURCES += $$PWD/qsql_ibase.cpp $$PWD/main.cpp

# FIXME: ignores libfb (unix)/fbclient (win32) - but that's for the test anyway
QMAKE_USE += ibase

OTHER_FILES += ibase.json

PLUGIN_CLASS_NAME = QIBaseDriverPlugin
include(../qsqldriverbase.pri)
