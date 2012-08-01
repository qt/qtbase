QT = core core-private testlib
TARGET = tst_qurlquery
CONFIG += parallel_test testcase
SOURCES += tst_qurlquery.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
