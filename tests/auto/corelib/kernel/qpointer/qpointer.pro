CONFIG += testcase
TARGET = tst_qpointer
QT = core testlib
QT += core-private
qtHaveModule(widgets): QT += widgets
SOURCES = tst_qpointer.cpp
