load(qttest_p4)
QT += printsupport widgets
SOURCES  += tst_qprinter.cpp

mac*:CONFIG+=insignificant_test
CONFIG += insignificant_test # QTBUG-21402
