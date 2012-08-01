CONFIG += testcase parallel_test
TARGET = tst_qmetaobjectbuilder
QT = core-private gui-private testlib
SOURCES = tst_qmetaobjectbuilder.cpp
macx:CONFIG -= app_bundle
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
