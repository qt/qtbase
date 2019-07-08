CONFIG += testcase
TARGET = tst_qmap
QT = core testlib
SOURCES = $$PWD/tst_qmap.cpp

DEFINES -= QT_NO_JAVA_STYLE_ITERATORS
