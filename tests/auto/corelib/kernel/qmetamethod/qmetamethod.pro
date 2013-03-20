CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qmetamethod
QT = core testlib
SOURCES = tst_qmetamethod.cpp
mac:CONFIG -= app_bundle
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
