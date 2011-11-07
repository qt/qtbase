CONFIG += testcase parallel_test
TARGET = tst_qsemaphore
QT = core testlib
SOURCES = tst_qsemaphore.cpp

mac*:CONFIG+=insignificant_test
