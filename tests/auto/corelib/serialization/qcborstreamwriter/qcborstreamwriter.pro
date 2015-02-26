QT = core testlib
TARGET = tst_qcborstreamwriter
CONFIG += testcase
SOURCES += \
    tst_qcborstreamwriter.cpp

INCLUDEPATH += ../../../../../src/3rdparty/tinycbor/tests/encoder
DEFINES += SRCDIR=\\\"$$PWD/\\\"
