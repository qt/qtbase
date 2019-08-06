CONFIG += testcase
TARGET = tst_containerapisymmetry
SOURCES  += tst_containerapisymmetry.cpp
QT = core testlib

# This test does not work with strict iterators
DEFINES -= QT_STRICT_ITERATORS
DEFINES -= QT_NO_LINKED_LIST
