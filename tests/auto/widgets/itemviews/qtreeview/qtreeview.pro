CONFIG += testcase
TARGET = tst_qtreeview
QT += widgets testlib
QT += widgets-private gui-private core-private
SOURCES  += tst_qtreeview.cpp
HEADERS  += ../../../../shared/fakedirmodel.h

win32: CONFIG += insignificant_test
