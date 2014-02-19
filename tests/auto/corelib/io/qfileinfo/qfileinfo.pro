CONFIG += testcase
TARGET = tst_qfileinfo
QT = core-private testlib
SOURCES = tst_qfileinfo.cpp
RESOURCES += qfileinfo.qrc

TESTDATA += qfileinfo.qrc qfileinfo.pro tst_qfileinfo.cpp resources/file1 resources/file1.ext1 resources/file1.ext1.ext2

win32*:!wince*:!winrt:LIBS += -ladvapi32 -lnetapi32
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
