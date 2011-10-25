QT += widgets testlib
QT += gui-private widgets-private

TEMPLATE = app
TARGET = tst_bench_QText

SOURCES += main.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
