CONFIG += testcase
TARGET = tst_qhash
QT = core testlib
SOURCES = $$PWD/tst_qhash.cpp

DEFINES -= QT_NO_JAVA_STYLE_ITERATORS
