CONFIG += testcase
TARGET = tst_qtreeview
QT += widgets testlib
QT += widgets-private gui-private core-private
SOURCES  += tst_qtreeview.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
