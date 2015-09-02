CONFIG += testcase
TARGET = tst_qstatemachine
QT = core-private testlib
qtHaveModule(widgets): QT += widgets
SOURCES = tst_qstatemachine.cpp
