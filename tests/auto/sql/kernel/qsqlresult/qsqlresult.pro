TARGET = tst_qsqlresult
CONFIG += testcase

QT = core sql testlib

SOURCES += tst_qsqlresult.cpp
HEADERS += testsqldriver.h

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
