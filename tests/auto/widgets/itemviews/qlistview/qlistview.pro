CONFIG += testcase
TARGET = tst_qlistview
QT += widgets gui-private widgets-private core-private testlib testlib-private
SOURCES  += tst_qlistview.cpp
win32:!winrt: QMAKE_USE += user32
