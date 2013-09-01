CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qeventloop
QT = core network testlib core-private
SOURCES = $$PWD/tst_qeventloop.cpp

win32:!wince*:!winrt:LIBS += -luser32
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

contains(QT_CONFIG, glib): DEFINES += HAVE_GLIB
