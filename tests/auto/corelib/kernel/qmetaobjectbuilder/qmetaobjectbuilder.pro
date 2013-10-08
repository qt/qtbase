CONFIG += testcase parallel_test
TARGET = tst_qmetaobjectbuilder
QT = core-private testlib
SOURCES = tst_qmetaobjectbuilder.cpp
mac:CONFIG -= app_bundle
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
