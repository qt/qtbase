QT = core-private testlib
TARGET = tst_qcborvalue
CONFIG += testcase
SOURCES += \
        tst_qcborvalue.cpp

INCLUDEPATH += \
        ../../../../../src/3rdparty/tinycbor/src \
        ../../../../../src/3rdparty/tinycbor/tests/parser

DEFINES += SRCDIR=\\\"$$PWD/\\\"
