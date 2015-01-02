CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qtransform
SOURCES  += tst_qtransform.cpp
QT += testlib

unix:!mac:!haiku:LIBS+=-lm
