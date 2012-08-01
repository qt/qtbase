CONFIG += testcase
TARGET = tst_qdir
QT = core core-private testlib
SOURCES = tst_qdir.cpp
RESOURCES += qdir.qrc

TESTDATA += testdir testData searchdir resources entrylist types tst_qdir.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
