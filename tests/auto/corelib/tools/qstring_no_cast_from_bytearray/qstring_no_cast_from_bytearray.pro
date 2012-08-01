CONFIG += testcase parallel_test
TARGET = tst_qstring_no_cast_from_bytearray
QT = core testlib
SOURCES = tst_qstring_no_cast_from_bytearray.cpp
DEFINES += QT_NO_CAST_FROM_BYTEARRAY

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
