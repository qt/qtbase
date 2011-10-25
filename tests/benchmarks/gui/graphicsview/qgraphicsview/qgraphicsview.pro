QT += widgets testlib

TEMPLATE = app
TARGET = tst_bench_qgraphicsview

SOURCES += tst_qgraphicsview.cpp
RESOURCES += qgraphicsview.qrc

include(chiptester/chiptester.pri)
