CONFIG += testcase
TARGET = tst_qfuture
QT = core core-private testlib
SOURCES = tst_qfuture.cpp
DEFINES += QT_STRICT_ITERATORS
DEFINES -= QT_NO_JAVA_STYLE_ITERATORS
