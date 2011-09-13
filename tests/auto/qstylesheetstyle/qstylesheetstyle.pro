load(qttest_p4)
QT += widgets widgets-private
QT += gui-private
# Input
SOURCES += tst_qstylesheetstyle.cpp
RESOURCES += resources.qrc
requires(contains(QT_CONFIG,private_tests))
CONFIG += insignificant_test #See QTBUG-21424
