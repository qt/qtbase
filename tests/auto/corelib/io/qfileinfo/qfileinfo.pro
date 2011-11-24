CONFIG += testcase parallel_test
TARGET = tst_qfileinfo
QT = core-private testlib
SOURCES = tst_qfileinfo.cpp
RESOURCES += qfileinfo.qrc

TESTDATA += qfileinfo.qrc qfileinfo.pro tst_qfileinfo.cpp resources/file1 resources/file1.ext1 resources/file1.ext1.ext2

win32*:LIBS += -ladvapi32 -lnetapi32
