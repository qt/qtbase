load(qttest_p4)

SOURCES   += tst_qfilesystementry.cpp \
    $${QT.core.sources}/io/qfilesystementry.cpp
HEADERS += $${QT.core.sources}/io/qfilesystementry_p.h
QT = core core-private

CONFIG += parallel_test
