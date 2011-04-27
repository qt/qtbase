load(qttest_p4)
SOURCES += tst_qobjectrace.cpp
QT = core

TARGET.EPOCHEAPSIZE = 20000000 40000000
CONFIG += parallel_test
