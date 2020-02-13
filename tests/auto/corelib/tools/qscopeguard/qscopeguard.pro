CONFIG += testcase
TARGET = tst_qscopeguard
QT = core testlib
SOURCES = tst_qscopeguard.cpp

# Force C++17 if available
contains(QT_CONFIG, c++1z): CONFIG += c++1z
