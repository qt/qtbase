CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qmetamethod
QT = core testlib
SOURCES = tst_qmetamethod.cpp
macx:CONFIG -= app_bundle
