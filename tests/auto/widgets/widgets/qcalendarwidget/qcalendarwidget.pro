CONFIG += testcase
TARGET = tst_qcalendarwidget
QT += widgets testlib
SOURCES  += tst_qcalendarwidget.cpp

# QTBUG-23615 - unstable test
linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):CONFIG += insignificant_test
