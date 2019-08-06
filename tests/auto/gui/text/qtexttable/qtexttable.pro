CONFIG += testcase
TARGET = tst_qtexttable
QT += testlib gui-private
qtHaveModule(widgets): QT += widgets
SOURCES  += tst_qtexttable.cpp



