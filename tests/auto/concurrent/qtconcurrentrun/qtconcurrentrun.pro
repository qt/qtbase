CONFIG += testcase
TARGET = tst_qtconcurrentrun
QT = core testlib concurrent
SOURCES = tst_qtconcurrentrun.cpp

# Force C++17 if available
contains(QT_CONFIG, c++1z): CONFIG += c++1z
