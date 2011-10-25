CONFIG += testcase
TARGET = tst_qstylesheetstyle
QT += widgets widgets-private testlib
QT += gui-private
# Input
SOURCES += tst_qstylesheetstyle.cpp
RESOURCES += resources.qrc
requires(contains(QT_CONFIG,private_tests))
