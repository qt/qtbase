CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qwmatrix
SOURCES  += tst_qwmatrix.cpp
QT += testlib

unix:!mac:LIBS+=-lm
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
