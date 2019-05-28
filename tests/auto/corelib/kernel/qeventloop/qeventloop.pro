CONFIG += testcase
TARGET = tst_qeventloop
QT = core network testlib core-private
SOURCES = $$PWD/tst_qeventloop.cpp

win32:!winrt: QMAKE_USE += user32

qtConfig(glib): DEFINES += HAVE_GLIB
