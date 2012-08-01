############################################################
# Project file for autotest for file qnetworkproxy.h (proxy factory part)
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qnetworkproxyfactory
QT = core network testlib

SOURCES += tst_qnetworkproxyfactory.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
