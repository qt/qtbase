CONFIG += testcase
TARGET = tst_collections
SOURCES  += tst_collections.cpp
QT = core testlib
CONFIG += parallel_test

# This test does not work with strict iterators
DEFINES -= QT_STRICT_ITERATORS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
