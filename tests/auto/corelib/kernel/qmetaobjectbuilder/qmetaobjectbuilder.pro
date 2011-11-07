CONFIG += testcase parallel_test
TARGET = tst_qmetaobjectbuilder
QT = core-private gui-private testlib
SOURCES = tst_qmetaobjectbuilder.cpp
macx:CONFIG -= app_bundle
