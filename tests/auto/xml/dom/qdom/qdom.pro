CONFIG += testcase
TARGET = tst_qdom
SOURCES  += tst_qdom.cpp

QT = core xml testlib

TESTDATA += testdata/* doubleNamespaces.xml umlaut.xml
