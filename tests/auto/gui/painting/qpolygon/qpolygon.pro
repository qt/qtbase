CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qpolygon
QT += testlib
SOURCES  += tst_qpolygon.cpp

unix:!mac:LIBS+=-lm


DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
