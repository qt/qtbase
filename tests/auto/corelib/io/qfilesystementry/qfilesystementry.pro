CONFIG += testcase
TARGET = tst_qfilesystementry
QT = core-private testlib
SOURCES = tst_qfilesystementry.cpp \
    $$QT_SOURCE_TREE/src/corelib/io/qfilesystementry.cpp
HEADERS = $$QT_SOURCE_TREE/src/corelib/io/qfilesystementry_p.h
