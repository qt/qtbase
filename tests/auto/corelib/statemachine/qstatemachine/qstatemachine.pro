CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qstatemachine
QT = core-private testlib gui
qtHaveModule(widgets): QT += widgets
SOURCES = tst_qstatemachine.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
