QT = core-private testlib
TARGET = tst_qcborstreamreader
CONFIG += testcase
SOURCES += \
    tst_qcborstreamreader.cpp

INCLUDEPATH += \
    ../../../../../src/3rdparty/tinycbor/src \
    ../../../../../src/3rdparty/tinycbor/tests/parser

DEFINES += SRCDIR=\\\"$$PWD/\\\"
