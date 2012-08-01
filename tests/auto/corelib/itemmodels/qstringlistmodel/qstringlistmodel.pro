CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qstringlistmodel
QT = core testlib
HEADERS += qmodellistener.h
SOURCES += tst_qstringlistmodel.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
