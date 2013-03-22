TARGET = tst_qsqlresult
CONFIG += testcase

QT = core sql testlib

SOURCES += tst_qsqlresult.cpp
HEADERS += testsqldriver.h

win32-g++*: LIBS += -lws2_32

