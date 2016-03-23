CONFIG += testcase
TARGET = tst_qeventloop
QT = core network testlib core-private
SOURCES = $$PWD/tst_qeventloop.cpp

win32:!winrt: LIBS += -luser32

contains(QT_CONFIG, glib): DEFINES += HAVE_GLIB
