CONFIG += testcase
osx: CONFIG += insignificant_test # QTBUG-46266, crashes
SOURCES=tst_qtouchevent.cpp
TARGET=tst_qtouchevent
QT += testlib widgets gui-private
