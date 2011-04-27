load(qttest_p4)

# Input
SOURCES += tst_qstylesheetstyle.cpp
RESOURCES += resources.qrc
requires(contains(QT_CONFIG,private_tests))
