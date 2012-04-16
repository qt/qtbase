CONFIG += testcase
TARGET = tst_qcombobox
QT += widgets widgets-private gui-private core-private testlib
SOURCES  += tst_qcombobox.cpp

linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):CONFIG += insignificant_test # QTBUG-23639, (related QTBUG-1071)
