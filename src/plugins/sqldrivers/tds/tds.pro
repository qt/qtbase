TARGET = qsqltds

HEADERS += $$PWD/qsql_tds_p.h
SOURCES += $$PWD/qsql_tds.cpp $$PWD/main.cpp

QMAKE_USE += tds

OTHER_FILES += tds.json

PLUGIN_CLASS_NAME = QTDSDriverPlugin
include(../qsqldriverbase.pri)
