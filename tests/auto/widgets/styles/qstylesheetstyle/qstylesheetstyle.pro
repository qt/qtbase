CONFIG += testcase
TARGET = tst_qstylesheetstyle
QT += widgets widgets-private gui-private testlib

SOURCES += tst_qstylesheetstyle.cpp
RESOURCES += resources.qrc

requires(contains(QT_CONFIG,private_tests))
