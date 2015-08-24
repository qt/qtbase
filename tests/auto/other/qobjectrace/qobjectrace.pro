CONFIG += testcase
SOURCES += tst_qobjectrace.cpp
QT = core testlib

TARGET.EPOCHEAPSIZE = 20000000 40000000
CONFIG += parallel_test
