CONFIG += testcase
TARGET = tst_containerapisymmetry
SOURCES  += tst_containerapisymmetry.cpp
QT = core testlib
contains(QT_CONFIG, c++2a): CONFIG += c++2a
