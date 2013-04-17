CONFIG += testcase parallel_test
TARGET = tst_qtranslator
QT = core testlib
SOURCES = tst_qtranslator.cpp
RESOURCES += qtranslator.qrc

TESTDATA += hellotr_la.qm msgfmt_from_po.qm
