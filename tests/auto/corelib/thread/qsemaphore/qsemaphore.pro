CONFIG += testcase
TARGET = tst_qsemaphore
SOURCES  += tst_qsemaphore.cpp
QT = core testlib


CONFIG += parallel_test
mac*:CONFIG+=insignificant_test
