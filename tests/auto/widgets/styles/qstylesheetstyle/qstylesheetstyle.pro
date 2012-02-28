CONFIG += testcase
TARGET = tst_qstylesheetstyle
QT += widgets widgets-private gui-private testlib

SOURCES += tst_qstylesheetstyle.cpp
RESOURCES += resources.qrc

requires(contains(QT_CONFIG,private_tests))

win32:CONFIG += insignificant_test # QTBUG-24323
