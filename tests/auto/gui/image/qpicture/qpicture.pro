CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qpicture
QT += testlib
qtHaveModule(widgets): QT += widgets
SOURCES  += tst_qpicture.cpp



