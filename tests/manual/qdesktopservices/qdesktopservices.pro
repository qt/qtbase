QT       += testlib

TARGET = tst_qdesktopservices
CONFIG   += cmdline

TEMPLATE = app


SOURCES += tst_qdesktopservices.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

OTHER_FILES += \
    test.txt
