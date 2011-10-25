CONFIG += testcase
TARGET = tst_qfilesystementry

SOURCES   += tst_qfilesystementry.cpp \
    $${QT.core.sources}/io/qfilesystementry.cpp
HEADERS += $${QT.core.sources}/io/qfilesystementry_p.h
QT = core core-private testlib

CONFIG += parallel_test
