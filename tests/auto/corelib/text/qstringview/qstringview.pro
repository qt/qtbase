CONFIG += testcase
TARGET = tst_qstringview
QT = core testlib
contains(QT_CONFIG, c++14):CONFIG *= c++14
contains(QT_CONFIG, c++1z):CONFIG *= c++1z
SOURCES += tst_qstringview.cpp
