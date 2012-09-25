CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qtemporaryfile
QT = core testlib
SOURCES = tst_qtemporaryfile.cpp
TESTDATA += tst_qtemporaryfile.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
RESOURCES += qtemporaryfile.qrc