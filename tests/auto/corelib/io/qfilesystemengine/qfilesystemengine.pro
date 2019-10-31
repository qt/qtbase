CONFIG += testcase
TARGET = tst_qfilesystemengine
QT = core-private testlib
SOURCES = tst_qfilesystemengine.cpp \
    $$QT_SOURCE_TREE/src/corelib/io/qfilesystementry.cpp
HEADERS = $$QT_SOURCE_TREE/src/corelib/io/qfilesystementry_p.h
