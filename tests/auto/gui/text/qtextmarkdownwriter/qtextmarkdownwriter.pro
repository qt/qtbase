CONFIG += testcase
TARGET = tst_qtextmarkdownwriter
QT += core-private gui-private testlib
SOURCES += tst_qtextmarkdownwriter.cpp
TESTDATA += \
    data/example.md \
    data/blockquotes.md \

DEFINES += SRCDIR=\\\"$$PWD\\\"
