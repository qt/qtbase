load(qttest_p4)

SOURCES   += tst_qfilesystementry.cpp \
    ../../../src/corelib/io/qfilesystementry.cpp
HEADERS += ../../../src/corelib/io/qfilesystementry_p.h
QT = core core-private

CONFIG += parallel_test
