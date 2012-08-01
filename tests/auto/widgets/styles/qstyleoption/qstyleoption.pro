TEMPLATE = app
TARGET = tst_qstyleoption

CONFIG += testcase
CONFIG += parallel_test
QT += widgets testlib

SOURCES += tst_qstyleoption.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
