CONFIG += testcase
TARGET = tst_qtimer
QT = core core-private testlib
SOURCES = tst_qtimer.cpp

# Force C++17 if available
contains(QT_CONFIG, c++1z): CONFIG += c++1z
