CONFIG += testcase
testcase.timeout = 500 # this test is slow
TARGET = tst_qgraphicsview

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qgraphicsview.cpp tst_qgraphicsview_2.cpp
HEADERS +=  tst_qgraphicsview.h
DEFINES += QT_NO_CAST_TO_ASCII

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
