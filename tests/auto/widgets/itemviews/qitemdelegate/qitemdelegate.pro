CONFIG += testcase
TARGET = tst_qitemdelegate
QT += widgets widgets-private testlib
SOURCES         += tst_qitemdelegate.cpp

win32:!winrt: QMAKE_USE += user32
