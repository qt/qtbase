CONFIG += testcase
TARGET = tst_qwmatrix
SOURCES  += tst_qwmatrix.cpp
QT += testlib

unix:!darwin:!haiku:!integrity: LIBS += -lm
