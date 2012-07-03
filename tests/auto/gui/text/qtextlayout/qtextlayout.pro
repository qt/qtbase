CONFIG += testcase
TARGET = tst_qtextlayout
QT += core-private gui-private testlib
HEADERS += 
SOURCES += tst_qtextlayout.cpp 
DEFINES += QT_COMPILES_IN_HARFBUZZ
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src
