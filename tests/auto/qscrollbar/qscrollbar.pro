load(qttest_p4)
QT += widgets
SOURCES += tst_qscrollbar.cpp

mac*:CONFIG+=insignificant_test
CONFIG += insignificant_test # QTBUG-21402
