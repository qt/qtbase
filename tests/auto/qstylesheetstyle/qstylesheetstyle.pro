load(qttest_p4)
QT += gui-private
# Input
SOURCES += tst_qstylesheetstyle.cpp
RESOURCES += resources.qrc
requires(contains(QT_CONFIG,private_tests))
