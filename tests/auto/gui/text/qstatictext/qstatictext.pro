CONFIG += testcase
TARGET = tst_qstatictext
QT += testlib
QT += core core-private gui gui-private
SOURCES  += tst_qstatictext.cpp

mac: CONFIG += insignificant_test # QTBUG-23063
