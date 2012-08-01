CONFIG += testcase parallel_test
TARGET = tst_qfilesystementry
QT = core-private testlib
SOURCES = tst_qfilesystementry.cpp \
    $${QT.core.sources}/io/qfilesystementry.cpp
HEADERS = $${QT.core.sources}/io/qfilesystementry_p.h
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
