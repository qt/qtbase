CONFIG += testcase
TARGET = tst_qpolygon
QT += widgets testlib
SOURCES  += tst_qpolygon.cpp

unix:!mac:LIBS+=-lm


