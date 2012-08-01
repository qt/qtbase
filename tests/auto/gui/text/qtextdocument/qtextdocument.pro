CONFIG += testcase
TARGET = tst_qtextdocument
QT += core-private gui-private xml testlib
HEADERS += common.h
SOURCES += tst_qtextdocument.cpp 


DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
