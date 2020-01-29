CONFIG += testcase
TARGET = tst_qtconcurrentmap
QT = core testlib concurrent
SOURCES = tst_qtconcurrentmap.cpp

# Force C++17 if available
contains(QT_CONFIG, c++1z): CONFIG += c++1z
