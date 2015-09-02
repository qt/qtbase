CONFIG += testcase
TARGET = tst_qfont
QT += testlib
QT += core-private gui-private
qtHaveModule(widgets): QT += widgets
SOURCES  += tst_qfont.cpp
