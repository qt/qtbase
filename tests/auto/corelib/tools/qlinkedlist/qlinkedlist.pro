CONFIG += testcase
TARGET = tst_qlinkedlist
QT = core testlib
SOURCES = tst_qlinkedlist.cpp
DEFINES -= QT_NO_LINKED_LIST
