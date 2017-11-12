CONFIG += testcase
TARGET = tst_qstylesheetstyle
QT += widgets widgets-private gui-private testlib testlib-private

SOURCES += tst_qstylesheetstyle.cpp
RESOURCES += resources.qrc

requires(qtConfig(private_tests))
