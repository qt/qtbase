CONFIG += testcase
TARGET = tst_qfontmetrics
QT += testlib
SOURCES  += tst_qfontmetrics.cpp
RESOURCES += testfont.qrc

win32:CONFIG += insignificant_test # QTBUG-24195
