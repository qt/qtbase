CONFIG += testcase
TARGET = tst_qdiriterator
QT = core-private core testlib
SOURCES = tst_qdiriterator.cpp
RESOURCES += qdiriterator.qrc

TESTDATA += entrylist
contains(CONFIG, builtin_testdata): DEFINES += BUILTIN_TESTDATA
