QT       += testlib

TARGET = tst_qdesktopservices
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += tst_qdesktopservices.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

OTHER_FILES += \
    test.txt
