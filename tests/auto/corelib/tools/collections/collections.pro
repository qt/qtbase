CONFIG += testcase
TARGET = tst_collections
SOURCES  += tst_collections.cpp
QT = core testlib

# This test does not work with strict iterators
DEFINES -= QT_STRICT_ITERATORS
