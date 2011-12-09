CONFIG += testcase
TARGET = tst_qtextscriptengine

QT += core-private gui-private testlib

HEADERS += 
SOURCES += tst_qtextscriptengine.cpp 
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

mac: CONFIG += insignificant_test # QTBUG-23064
