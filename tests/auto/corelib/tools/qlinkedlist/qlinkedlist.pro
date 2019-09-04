CONFIG += testcase
TARGET = tst_qlinkedlist
QT = core testlib
qtConfig(c++14): CONFIG += c++14
qtConfig(c++1z): CONFIG += c++1z
SOURCES = tst_qlinkedlist.cpp
DEFINES -= QT_NO_LINKED_LIST
