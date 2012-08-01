QT += widgets testlib
QT += gui-private widgets-private

TEMPLATE = app
TARGET = tst_bench_qpainter

SOURCES += tst_qpainter.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
