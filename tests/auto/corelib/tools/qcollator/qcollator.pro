CONFIG += testcase parallel_test
TARGET = tst_qcollator
QT = core testlib
SOURCES = tst_qcollator.cpp
DEFINES += QT_NO_CAST_TO_ASCII
contains(QT_CONFIG,icu):DEFINES += QT_USE_ICU
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
