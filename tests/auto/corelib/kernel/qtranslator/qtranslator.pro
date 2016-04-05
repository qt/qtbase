CONFIG += testcase
TARGET = tst_qtranslator
QT = core testlib
SOURCES = tst_qtranslator.cpp
RESOURCES += qtranslator.qrc

android: RESOURCES += android_testdata.qrc
else: TESTDATA += dependencies_la.qm hellotr_la.qm msgfmt_from_po.qm

