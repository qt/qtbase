CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qtransform
SOURCES  += tst_qtransform.cpp
QT += testlib

unix:!mac:LIBS+=-lm
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
