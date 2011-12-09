CONFIG += testcase
TARGET = tst_qkeysequence

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES  += tst_qkeysequence.cpp

RESOURCES += qkeysequence.qrc

mac: CONFIG += insignificant_test # QTBUG-23058
