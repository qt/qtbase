CONFIG += testcase
TARGET = tst_qcombobox
QT += widgets widgets-private gui-private core-private testlib
SOURCES  += tst_qcombobox.cpp

# QTBUG-23639 - unstable test (related QTBUG-1071)
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):CONFIG += insignificant_test
