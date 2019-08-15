CONFIG += testcase
TARGET = tst_qbytearraymatcher
QT = core testlib
SOURCES = tst_qbytearraymatcher.cpp
contains(QT_CONFIG, c++14):CONFIG += c++14
