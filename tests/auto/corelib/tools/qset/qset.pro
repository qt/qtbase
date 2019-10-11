CONFIG += testcase
TARGET = tst_qset
QT = core testlib
qtConfig(c++14): CONFIG += c++14
qtConfig(c++1z): CONFIG += c++1z
SOURCES = tst_qset.cpp

DEFINES -= QT_NO_JAVA_STYLE_ITERATORS
