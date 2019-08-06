CONFIG += testcase
TARGET = tst_qtconcurrentmap
QT = core testlib concurrent
SOURCES = tst_qtconcurrentmap.cpp
DEFINES += QT_STRICT_ITERATORS
DEFINES -= QT_NO_LINKED_LIST

# Force C++17 if available
contains(QT_CONFIG, c++1z): CONFIG += c++1z
