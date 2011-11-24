CONFIG += testcase
TARGET = tst_qtranslator
QT += widgets testlib
SOURCES = tst_qtranslator.cpp
RESOURCES += qtranslator.qrc

TESTDATA += hellotr_la.qm msgfmt_from_po.qm
