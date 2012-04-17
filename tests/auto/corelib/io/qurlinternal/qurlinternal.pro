CONFIG += testcase
TARGET = tst_qurlinternal
SOURCES += tst_qurlinternal.cpp ../../codecs/utf8/utf8data.cpp
QT = core core-private testlib
CONFIG += parallel_test
