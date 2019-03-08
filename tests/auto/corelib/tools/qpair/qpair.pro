CONFIG += testcase
TARGET = tst_qpair
QT = core testlib
SOURCES = tst_qpair.cpp

# Force C++17 if available (needed due to Q_COMPILER_DEDUCTION_GUIDES)
contains(QT_CONFIG, c++1z): CONFIG += c++1z
