CONFIG += testcase
TARGET = tst_qfiledialog2

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES += tst_qfiledialog2.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
