CONFIG += testcase
requires(contains(QT_CONFIG,private_tests))
TARGET = tst_qurlinternal
SOURCES += tst_qurlinternal.cpp ../../codecs/utf8/utf8data.cpp
QT = core core-private testlib
CONFIG += parallel_test
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
